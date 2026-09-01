# 04 — Monitor de tráfego TCP por porta (exemplo principal, C+libbpf/CO-RE e Python+BCC)

Este é o exemplo central do minicurso: replica, de forma simplificada, o
coletor descrito na seção 7 do esqueleto do curso e usado no experimento do
TCC para observar o tráfego de um WAF em container Docker.

Duas kprobes cobrem as duas direções do tráfego TCP de qualquer conexão do
sistema (incluindo as originadas dentro de containers, seção 7.6):

- `tcp_sendmsg` — cada envio de dados;
- `tcp_cleanup_rbuf` — cada entrega de dados recebidos à aplicação.

Cada uma acumula bytes em um `BPF_MAP_TYPE_HASH` chaveado pela **porta local**
da conexão (`sk->__sk_common.skc_num`, lida com `BPF_CORE_READ` para suportar
CO-RE — seção 3.5). O programa de controle lê os dois mapas a cada segundo e
imprime uma tabela `porta | enviados | recebidos`.

> Nota: o esqueleto do curso mostra o campo `skc_dport` (porta remota) no
> trecho de código ilustrativo da seção 7.1. Usamos `skc_num` (porta local)
> aqui para que a tabela sempre mostre a porta do processo observado — no
> laboratório, `8080` do `toy-server` — em vez da porta efêmera do cliente
> que muda a cada conexão.

Existe também uma versão equivalente em Python + BCC em `python/monitor.py`,
com a mesma lógica (duas kprobes, dois `BPF_HASH` chaveados por porta). A
diferença notável: o BCC compila contra os cabeçalhos reais do kernel
instalados na máquina, então o acesso a `sk->__sk_common.skc_num` não precisa
de `BPF_CORE_READ` — não há problema de portabilidade entre kernels porque o
programa é recompilado a cada execução, para o kernel local (é exatamente o
que a seção 3.5 descreve como motivação para o CO-RE: evitar essa
recompilação em cada máquina).

## Rodando

Com o ambiente Docker (ver `docker/README.md`), em um terminal suba o
`toy-server` e, no container `dev`, compile e rode o monitor:

```bash
# fora do container: garante que o toy-server está de pé
docker compose up -d toy-server

# dentro do container dev — versão C + libbpf/CO-RE
cd examples/04-tcp-monitor
make
sudo ./build/monitor

# ou a versão Python + BCC, equivalente
sudo python3 python/monitor.py
```

Em outro terminal, gere tráfego **de dentro de outro container na mesma rede
docker** (não do host, nem pelo IP do container direto do host — o
`docker-proxy` que publica a porta introduz um salto que não aparece com a
porta 8080 nos contadores, já que o kprobe vê a chamada de kernel de quem
originou a conexão, não a porta publicada):

```bash
docker compose exec dev bash
./scripts/gen-traffic.sh toy-server 8080 100
```

A linha da porta `8080` na tabela deve crescer nas duas colunas a cada
requisição.

> **Status de teste:** ambas as versões foram compiladas/carregadas e
> validadas ponta a ponta (contadores da porta 8080 crescendo conforme
> esperado), na VM libvirt (ver `../../libvirt/README.md`). A versão em
> Python precisava de um `#include <net/sock.h>` no programa embutido (sem
> ele, o BCC não vê a definição completa de `struct sock` e falha ao
> compilar) e de um pequeno ajuste em `imprimir_tabela()` (a tabela do BCC
> espera uma chave `ctypes`, não um `int` puro, no `.get()`) — ambos já
> corrigidos em `python/monitor.py`.

## O que observar

- Nenhum arquivo de `/proc` é lido, nenhum pacote é copiado para o espaço do
  usuário — o kernel agrega os contadores no próprio ponto onde os bytes são
  processados, e o espaço do usuário só lê o resultado já pronto quando quer
  (seção 5.3.1).
- O monitor roda no host e enxerga o tráfego do `toy-server`, que roda
  isolado em outro container, sem nenhuma configuração de rede especial —
  kprobes observam funções do kernel, que é compartilhado entre host e
  todos os containers (seção 7.6).
- Compare o `htop`/CPU consumido por este coletor com o de uma ferramenta
  baseada em polling (ex: `watch -n1 cat /proc/net/dev`) sob a mesma carga
  de tráfego — é a demonstração prática da seção 5.3.1 (menor overhead de
  CPU do eBPF).
