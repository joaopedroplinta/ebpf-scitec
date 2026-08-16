// Programa eBPF (espaço kernel): conta quantas vezes execve() foi chamado
// no sistema desde que o programa foi carregado. Ver seção 7.1 do esqueleto
// do curso para a anatomia geral de um programa eBPF em C.
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, __u64);
} contador SEC(".maps");

SEC("tracepoint/syscalls/sys_enter_execve")
int rastrear_execve(void *ctx)
{
    __u32 chave = 0;
    __u64 *total = bpf_map_lookup_elem(&contador, &chave);
    __u64 novo_total = total ? *total + 1 : 1;

    bpf_map_update_elem(&contador, &chave, &novo_total, BPF_ANY);
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
