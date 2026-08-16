// Exemplo principal do minicurso (seção 7 do esqueleto do curso): coletor de
// bytes transmitidos e recebidos por porta TCP, equivalente ao usado no
// experimento do TCC para observar o tráfego de um WAF em container.
//
// Duas kprobes cobrem as duas direções do tráfego:
//   - tcp_sendmsg:       disparada a cada envio de dados por uma conexão TCP
//   - tcp_cleanup_rbuf:  disparada quando dados recebidos são entregues à aplicação
//
// Usamos sk->__sk_common.skc_num (porta local, em host byte order) em vez de
// skc_dport (porta remota, network byte order) para que a porta que aparece
// no mapa seja sempre a do processo observado — no laboratório deste curso,
// a porta 8080 do toy-server — independentemente de quem iniciou a conexão.
#include "vmlinux.h"
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, __u16);   // porta TCP local
    __type(value, __u64); // bytes acumulados
} bytes_enviados SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, __u16);
    __type(value, __u64);
} bytes_recebidos SEC(".maps");

static __always_inline void acumular(void *mapa, __u16 porta, __u64 quantidade)
{
    __u64 *total = bpf_map_lookup_elem(mapa, &porta);
    __u64 novo_total = total ? *total + quantidade : quantidade;

    bpf_map_update_elem(mapa, &porta, &novo_total, BPF_ANY);
}

SEC("kprobe/tcp_sendmsg")
int BPF_KPROBE(trace_tcp_sendmsg, struct sock *sk, struct msghdr *msg, size_t size)
{
    __u16 porta = BPF_CORE_READ(sk, __sk_common.skc_num);

    acumular(&bytes_enviados, porta, size);
    return 0;
}

SEC("kprobe/tcp_cleanup_rbuf")
int BPF_KPROBE(trace_tcp_cleanup_rbuf, struct sock *sk, int copied)
{
    __u16 porta;

    if (copied <= 0)
        return 0;

    porta = BPF_CORE_READ(sk, __sk_common.skc_num);
    acumular(&bytes_recebidos, porta, (__u64)copied);
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
