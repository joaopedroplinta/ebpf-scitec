# 04 — Monitor de tráfego TCP por porta (exemplo principal)

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

## Rodando

Com o ambiente Docker (ver `docker/README.md`), em um terminal suba o
`toy-server` e, no container `dev`, compile e rode o monitor:

```bash
# fora do container: garante que o toy-server está de pé
docker compose up -d toy-server

# dentro do container dev
cd examples/04-tcp-monitor
make
sudo ./build/monitor
```

Em outro terminal, gere tráfego:

```bash
./scripts/gen-traffic.sh localhost 8080 100
```

A linha da porta `8080` na tabela deve crescer nas duas colunas a cada
requisição.

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
