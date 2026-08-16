#!/usr/bin/env python3
"""Equivalente em BCC de xdp_drop.bpf.c/xdp_drop.c (../src): mesmo parsing
manual de Ethernet/IP/TCP/UDP, mesma lógica de bloqueio por porta de destino,
mas carregado e anexado via BCC em vez de libbpf.

⚠️ Mesmo aviso de segurança do README desta pasta: nunca aponte para a
interface de rede principal de uma máquina remota ou compartilhada. Use um
par veth com a ponta remota em outro network namespace (ver README).

Uso: sudo python3 xdp_drop.py <interface> <porta_a_bloquear>
"""
import ctypes
import sys
from time import sleep

from bcc import BPF

programa = r"""
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>

BPF_ARRAY(porta_bloqueada, u16, 1);
BPF_ARRAY(pacotes_descartados, u64, 1);

int bloquear_porta(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    struct ethhdr *eth = data;
    struct iphdr *ip;
    u16 porta_destino;
    int chave = 0;

    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;
    if (eth->h_proto != htons(ETH_P_IP))
        return XDP_PASS;

    ip = (void *)(eth + 1);
    if ((void *)(ip + 1) > data_end)
        return XDP_PASS;

    if (ip->protocol == IPPROTO_TCP) {
        struct tcphdr *tcp = (void *)ip + (ip->ihl * 4);
        if ((void *)(tcp + 1) > data_end)
            return XDP_PASS;
        porta_destino = ntohs(tcp->dest);
    } else if (ip->protocol == IPPROTO_UDP) {
        struct udphdr *udp = (void *)ip + (ip->ihl * 4);
        if ((void *)(udp + 1) > data_end)
            return XDP_PASS;
        porta_destino = ntohs(udp->dest);
    } else {
        return XDP_PASS;
    }

    u16 *alvo = porta_bloqueada.lookup(&chave);
    if (!alvo || *alvo != porta_destino)
        return XDP_PASS;

    u64 *total = pacotes_descartados.lookup(&chave);
    u64 novo_total = total ? *total + 1 : 1;
    pacotes_descartados.update(&chave, &novo_total);

    return XDP_DROP;
}
"""

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"uso: {sys.argv[0]} <interface> <porta_a_bloquear>")
        sys.exit(1)

    iface = sys.argv[1]
    porta = int(sys.argv[2])

    b = BPF(text=programa)
    fn = b.load_func("bloquear_porta", BPF.XDP)
    b["porta_bloqueada"][ctypes.c_int(0)] = ctypes.c_uint16(porta)
    b.attach_xdp(iface, fn, flags=BPF.XDP_FLAGS_SKB_MODE)

    print(f"Bloqueando pacotes TCP/UDP com porta de destino {porta} em '{iface}'. Ctrl+C para sair.")
    try:
        while True:
            total = b["pacotes_descartados"][ctypes.c_int(0)].value
            print(f"\rpacotes descartados: {total}", end="", flush=True)
            sleep(1)
    except KeyboardInterrupt:
        print()
    finally:
        b.remove_xdp(iface, flags=BPF.XDP_FLAGS_SKB_MODE)
