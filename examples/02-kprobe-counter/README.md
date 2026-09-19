# 02 — Contador via kprobe (C+libbpf/CO-RE e Python+BCC)

Enquanto o exemplo `01-hello-tracepoint` se anexa a um ponto estático
(tracepoint), este se anexa **dinamicamente** a uma função interna do kernel
— `tcp_sendmsg` — usando uma kprobe (seção 4.1 do esqueleto do curso). Soma o
argumento `size` de cada chamada para manter um total de bytes enviados via
TCP em todo o sistema.

Repare que ler `size` não exige `BPF_CORE_READ`: é um argumento escalar
passado diretamente à função, não um campo de uma struct do kernel que possa
mudar de offset entre versões (isso só aparece no exemplo `04-tcp-monitor`,
quando lemos um campo de `struct sock`).

## Rodando (C + libbpf/CO-RE)

```bash
make
sudo ./build/counter
```

## Rodando (Python + BCC)

```bash
sudo python3 python/counter.py
```

Em ambos os casos, gere tráfego em outro terminal (`curl` contra o
`toy-server`, ou qualquer conexão TCP) e veja o total subir. Testado de
ponta a ponta nas duas versões.

## O que observar

- Na versão em C, `BPF_KPROBE(ao_enviar, struct sock *sk, struct msghdr *msg,
  size_t size)` é uma macro do libbpf que expande para ler os argumentos da
  função instrumentada a partir dos registradores da CPU no ponto de
  entrada. Na versão em Python, fazemos isso manualmente com
  `PT_REGS_PARM3(ctx)` — o equivalente ao 3º argumento (`size`) na convenção
  de chamada x86-64 — que é o estilo "clássico" do BCC, mais leve que
  receber `struct sock`/`struct msghdr` tipados (o que exigiria puxar os
  cabeçalhos completos do kernel só para um argumento escalar).
- Kprobes têm overhead maior que tracepoints em funções chamadas com muita
  frequência (seção 4.1 vs. 4.3) porque não são pontos de instrumentação
  pré-otimizados pelo kernel — vale a pena para casos como este, em que não
  existe tracepoint equivalente para o que se quer medir.

## Desafio

Este programa já conta **bytes** enviados via TCP. Sua tarefa: adicionar um
segundo contador que conta **quantidade de chamadas** de `tcp_sendmsg` (ou
seja, quantos envios aconteceram, não quantos bytes).

Use a versão Python (`python/counter.py`) como base:

1. Crie um segundo mapa `BPF_ARRAY(chamadas_totais, u64, 1)`.
2. Dentro de `ao_enviar()`, incremente esse mapa em 1 a cada chamada (mesma
   lógica do `bytes_totais`, só que soma `1` em vez de `size`).
3. No laço de impressão em Python, leia e mostre esse novo contador junto do
   total de bytes.

Teste gerando tráfego em outro terminal (`curl` contra o `toy-server`, ou
qualquer conexão TCP) e confira: o contador de chamadas deve crescer junto
com o de bytes, mas em ritmos diferentes (uma chamada pode carregar poucos
ou muitos bytes).

> **Não faça isso:** trocar `tcp_sendmsg` por `tcp_recvmsg` para "contar bytes
> recebidos por simetria". O 3º argumento de `tcp_recvmsg` é o tamanho do
> **buffer pedido** pela aplicação, não os bytes efetivamente recebidos — o
> resultado parece funcionar mas é enganoso. Contar bytes recebidos de
> verdade exige a técnica usada no exemplo `04-tcp-monitor` (`tcp_cleanup_rbuf`).
