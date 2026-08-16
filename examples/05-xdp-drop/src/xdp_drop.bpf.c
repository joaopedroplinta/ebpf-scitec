// Extra opcional (seção 4.5): descarta pacotes TCP/UDP destinados a uma
// porta configurável, no ponto mais precoce possível — antes do kernel
// alocar qualquer estrutura de memória para o pacote (XDP_DROP). Ao
// contrário das kprobes dos exemplos anteriores, aqui o parsing dos
// cabeçalhos (Ethernet/IP/TCP/UDP) é feito manualmente sobre os bytes
// brutos do pacote, sempre validando limites contra ctx->data_end — o
// verificador rejeita qualquer acesso que não prove estar dentro do buffer
// (seção 3.3).
#include "vmlinux.h"
#include <bpf/bpf_endian.h>
#include <bpf/bpf_helpers.h>

#define ETH_P_IP 0x0800
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17

// chave 0: porta de destino a bloquear (configurada pelo espaço do usuário
// antes de anexar o programa)
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, __u16);
} porta_bloqueada SEC(".maps");

// chave 0: total de pacotes descartados
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, __u64);
} pacotes_descartados SEC(".maps");

SEC("xdp")
int bloquear_porta(struct xdp_md *ctx)
{
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    struct ethhdr *eth = data;
    struct iphdr *ip;
    __u16 porta_destino;
    __u32 chave = 0;
    __u16 *alvo;
    __u64 *total, novo_total;

    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;
    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return XDP_PASS;

    ip = (void *)(eth + 1);
    if ((void *)(ip + 1) > data_end)
        return XDP_PASS;

    if (ip->protocol == IPPROTO_TCP) {
        struct tcphdr *tcp = (void *)ip + (ip->ihl * 4);
        if ((void *)(tcp + 1) > data_end)
            return XDP_PASS;
        porta_destino = bpf_ntohs(tcp->dest);
    } else if (ip->protocol == IPPROTO_UDP) {
        struct udphdr *udp = (void *)ip + (ip->ihl * 4);
        if ((void *)(udp + 1) > data_end)
            return XDP_PASS;
        porta_destino = bpf_ntohs(udp->dest);
    } else {
        return XDP_PASS;
    }

    alvo = bpf_map_lookup_elem(&porta_bloqueada, &chave);
    if (!alvo || *alvo != porta_destino)
        return XDP_PASS;

    total = bpf_map_lookup_elem(&pacotes_descartados, &chave);
    novo_total = total ? *total + 1 : 1;
    bpf_map_update_elem(&pacotes_descartados, &chave, &novo_total, BPF_ANY);

    return XDP_DROP;
}

char LICENSE[] SEC("license") = "GPL";
