// Mesma kprobe do exemplo 02, mas em vez de um único contador acumulado
// (BPF_MAP_TYPE_ARRAY com 1 entrada), usamos um mapa com 32 posições para
// construir um histograma em potências de 2 do tamanho de cada envio TCP —
// o mesmo padrão usado por ferramentas como bpftrace para visualizar
// distribuições (seção 4.6 do esqueleto do curso, tipo ARRAY).
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

#define NUM_BUCKETS 32

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, NUM_BUCKETS);
    __type(key, __u32);
    __type(value, __u64);
} histograma SEC(".maps");

static __always_inline __u32 log2_aprox(__u32 v)
{
    __u32 r = 0;

#pragma unroll
    for (int i = 0; i < 31; i++) {
        if (v <= 1)
            break;
        v >>= 1;
        r++;
    }
    return r;
}

SEC("kprobe/tcp_sendmsg")
int BPF_KPROBE(rastrear_tamanho, struct sock *sk, struct msghdr *msg, size_t size)
{
    __u32 bucket = log2_aprox((__u32)size);
    __u64 *contagem;
    __u64 nova;

    if (bucket >= NUM_BUCKETS)
        bucket = NUM_BUCKETS - 1;

    contagem = bpf_map_lookup_elem(&histograma, &bucket);
    nova = contagem ? *contagem + 1 : 1;
    bpf_map_update_elem(&histograma, &bucket, &nova, BPF_ANY);
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
