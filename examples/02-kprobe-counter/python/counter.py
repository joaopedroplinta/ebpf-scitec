#!/usr/bin/env python3
"""Equivalente em BCC de counter.bpf.c/counter.c (../src): soma o argumento
"size" de cada chamada de tcp_sendmsg() em um contador global. Em vez de
receber os argumentos já tipados (o que exigiria puxar os cabeçalhos
completos do kernel para definir "struct sock"/"struct msghdr" -- desnecessário
aqui, já que só usamos "size"), lemos o 3º argumento diretamente do registrador
via PT_REGS_PARM3, seguindo a convenção de chamada x86-64 (sk, msg, size ->
rdi, rsi, rdx). É o estilo "clássico" do BCC, mais leve que o equivalente
tipado usado no exemplo 04 (que precisa mesmo de "struct sock" completo).

Uso: sudo python3 counter.py
"""
from time import sleep

from bcc import BPF

programa = r"""
#include <asm/ptrace.h>

BPF_ARRAY(bytes_totais, u64, 1);

int ao_enviar(struct pt_regs *ctx) {
    size_t size = (size_t)PT_REGS_PARM3(ctx);
    int chave = 0;
    u64 *total = bytes_totais.lookup(&chave);
    u64 novo_total = total ? *total + size : size;
    bytes_totais.update(&chave, &novo_total);
    return 0;
}
"""

if __name__ == "__main__":
    b = BPF(text=programa)
    b.attach_kprobe(event="tcp_sendmsg", fn_name="ao_enviar")

    print("Contando bytes enviados via TCP em todo o sistema... Ctrl+C para sair.")
    try:
        while True:
            total = b["bytes_totais"][0].value
            print(f"\rbytes enviados (tcp_sendmsg) desde o inicio: {total}", end="", flush=True)
            sleep(1)
    except KeyboardInterrupt:
        print()
