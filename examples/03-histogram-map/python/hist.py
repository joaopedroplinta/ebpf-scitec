#!/usr/bin/env python3
"""Mesma ideia de hist.bpf.c/hist.c (../src) -- histograma em potências de 2
do tamanho de cada tcp_sendmsg() -- mas usando BPF_HISTOGRAM, um tipo de mapa
que o BCC oferece pronto: ele já cuida do bucketing (bpf_log2l) e da
formatação em espaço de usuário (print_log2_hist). A versão em C faz esse
mesmo trabalho manualmente (função log2_aprox() + laço de impressão) porque
libbpf não tem esse açúcar sintático -- é um bom exemplo de como o BCC
prioriza produtividade e o libbpf/CO-RE prioriza portabilidade e menor
overhead de carregamento (ver README principal desta pasta). Assim como no
exemplo 02, lemos "size" direto do registrador (PT_REGS_PARM3) em vez de
receber um argumento tipado.

Uso: sudo python3 hist.py
"""
import sys
from time import sleep

from bcc import BPF

programa = r"""
#include <asm/ptrace.h>

BPF_HISTOGRAM(histograma);

int rastrear_tamanho(struct pt_regs *ctx) {
    size_t size = (size_t)PT_REGS_PARM3(ctx);
    histograma.increment(bpf_log2l(size));
    return 0;
}
"""

if __name__ == "__main__":
    b = BPF(text=programa)
    b.attach_kprobe(event="tcp_sendmsg", fn_name="rastrear_tamanho")

    print("Distribuindo tamanhos de tcp_sendmsg() em um histograma... Ctrl+C para sair.")
    try:
        while True:
            sleep(2)
            print("\033[2J\033[H", end="")
            b["histograma"].print_log2_hist("tamanho (bytes)")
            sys.stdout.flush()
    except KeyboardInterrupt:
        print()
