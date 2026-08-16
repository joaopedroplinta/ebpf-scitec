// Diferente do exemplo 01 (tracepoint, ponto estático), este programa se
// anexa dinamicamente a uma função do kernel via kprobe (seção 4.1) e lê um
// dos seus argumentos diretamente — sem precisar de CO-RE/BPF_CORE_READ,
// porque "size" é um argumento escalar da função, não um campo de struct do
// kernel.
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, __u64);
} bytes_totais SEC(".maps");

SEC("kprobe/tcp_sendmsg")
int BPF_KPROBE(ao_enviar, struct sock *sk, struct msghdr *msg, size_t size)
{
    __u32 chave = 0;
    __u64 *total = bpf_map_lookup_elem(&bytes_totais, &chave);
    __u64 novo_total = total ? *total + size : size;

    bpf_map_update_elem(&bytes_totais, &chave, &novo_total, BPF_ANY);
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
