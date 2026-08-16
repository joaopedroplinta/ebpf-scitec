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
