#!/usr/bin/env python3
"""Aquecimento: a cada chamada de execve() no sistema, imprime o PID que a
originou. Serve para mostrar, ao vivo, um programa eBPF rodando sem nenhum
passo de compilação manual — o BCC compila o texto C para bytecode eBPF em
tempo de execução usando o clang embutido na biblioteca.

Uso: sudo python3 hello_execve.py
"""
from bcc import BPF

program = r"""
TRACEPOINT_PROBE(syscalls, sys_enter_execve) {
    bpf_trace_printk("execve chamado por PID %d\n", bpf_get_current_pid_tgid() >> 32);
    return 0;
}
"""

if __name__ == "__main__":
    b = BPF(text=program)
    print("Rastreando execve() em todo o sistema... Ctrl+C para sair.")
    b.trace_print()
