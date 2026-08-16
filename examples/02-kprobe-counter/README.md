# 02 — Contador via kprobe

Enquanto o exemplo `01-hello-tracepoint` se anexa a um ponto estático
(tracepoint), este se anexa **dinamicamente** a uma função interna do kernel
— `tcp_sendmsg` — usando uma kprobe (seção 4.1 do esqueleto do curso). Soma o
argumento `size` de cada chamada para manter um total de bytes enviados via
TCP em todo o sistema.

Repare que ler `size` não exige `BPF_CORE_READ`: é um argumento escalar
passado diretamente à função, não um campo de uma struct do kernel que possa
mudar de offset entre versões (isso só aparece no exemplo `04-tcp-monitor`,
quando lemos um campo de `struct sock`).

## Rodando

```bash
make
sudo ./build/counter
```

Gere tráfego em outro terminal (`curl` contra o `toy-server`, ou qualquer
conexão TCP) e veja o total subir.

## O que observar

- `BPF_KPROBE(ao_enviar, struct sock *sk, struct msghdr *msg, size_t size)`
  é uma macro do libbpf que expande para ler os argumentos da função
  instrumentada a partir dos registradores da CPU no ponto de entrada —
  ela existe justamente para poupar o desenvolvedor de lidar manualmente
  com a convenção de chamada da arquitetura.
- Kprobes têm overhead maior que tracepoints em funções chamadas com muita
  frequência (seção 4.1 vs. 4.3) porque não são pontos de instrumentação
  pré-otimizados pelo kernel — vale a pena para casos como este, em que não
  existe tracepoint equivalente para o que se quer medir.
