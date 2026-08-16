#!/usr/bin/env python3
"""Equivalente em BCC do exemplo principal do minicurso (monitor.bpf.c/monitor.c
em ../src): duas kprobes (tcp_sendmsg e tcp_cleanup_rbuf) acumulando bytes em
mapas HASH chaveados pela porta local da conexão (sk->__sk_common.skc_num).

Diferença notável em relação à versão em C: o BCC compila contra os
cabeçalhos reais do kernel (instalados na máquina), então o acesso a
sk->__sk_common.skc_num não precisa de BPF_CORE_READ -- não há problema de
portabilidade entre kernels porque o programa é recompilado a cada execução,
para o kernel local (ver seção 3.5 do esqueleto do curso sobre a motivação
do CO-RE: evitar exatamente essa recompilação em cada máquina).

Uso: sudo python3 monitor.py
"""
import sys
from time import sleep

from bcc import BPF

programa = r"""
BPF_HASH(bytes_enviados, u16, u64);
BPF_HASH(bytes_recebidos, u16, u64);

int trace_tcp_sendmsg(struct pt_regs *ctx, struct sock *sk, struct msghdr *msg, size_t size) {
    u16 porta = sk->__sk_common.skc_num;
    u64 *total = bytes_enviados.lookup(&porta);
    u64 novo_total = total ? *total + size : size;
    bytes_enviados.update(&porta, &novo_total);
    return 0;
}

int trace_tcp_cleanup_rbuf(struct pt_regs *ctx, struct sock *sk, int copied) {
    if (copied <= 0)
        return 0;
    u16 porta = sk->__sk_common.skc_num;
    u64 *total = bytes_recebidos.lookup(&porta);
    u64 novo_total = total ? *total + copied : copied;
    bytes_recebidos.update(&porta, &novo_total);
    return 0;
}
"""


def imprimir_tabela(b):
    enviados = b["bytes_enviados"]
    recebidos = b["bytes_recebidos"]
    portas = {k.value for k in enviados.keys()} | {k.value for k in recebidos.keys()}

    print("\033[2J\033[H", end="")
    print(f"{'porta':<8} {'enviados(B)':>14} {'recebidos(B)':>14}")
    print("-" * 44)
    for porta in sorted(portas):
        e = enviados.get(porta)
        r = recebidos.get(porta)
        print(f"{porta:<8} {(e.value if e else 0):>14} {(r.value if r else 0):>14}")
    print("\n(Ctrl+C para sair)")
    sys.stdout.flush()


if __name__ == "__main__":
    b = BPF(text=programa)
    b.attach_kprobe(event="tcp_sendmsg", fn_name="trace_tcp_sendmsg")
    b.attach_kprobe(event="tcp_cleanup_rbuf", fn_name="trace_tcp_cleanup_rbuf")

    try:
        while True:
            imprimir_tabela(b)
            sleep(1)
    except KeyboardInterrupt:
        print()
