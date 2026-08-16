#!/usr/bin/env python3
"""Mesmo conceito de hello.bpf.c/hello.c (../src), só que via BCC: um
BPF_MAP_TYPE_ARRAY de verdade em vez do bpf_trace_printk usado no aquecimento
(../../00-bcc-warmup). Compara com o C: a lógica do programa eBPF em si
(contar execve() em um mapa) é idêntica, só muda quem compila (BCC compila em
tempo real; o C usa clang -target bpf + libbpf, uma vez só).

Uso: sudo python3 hello_execve.py
"""
from time import sleep

from bcc import BPF

programa = r"""
BPF_ARRAY(contador, u64, 1);

TRACEPOINT_PROBE(syscalls, sys_enter_execve) {
    int chave = 0;
    u64 *total = contador.lookup(&chave);
    u64 novo_total = total ? *total + 1 : 1;
    contador.update(&chave, &novo_total);
    return 0;
}
"""

if __name__ == "__main__":
    b = BPF(text=programa)
    print("Rastreando execve() em todo o sistema... Ctrl+C para sair.")
    try:
        while True:
            total = b["contador"][0].value
            print(f"\rtotal de execve() desde o inicio: {total}", end="", flush=True)
            sleep(1)
    except KeyboardInterrupt:
        print()
