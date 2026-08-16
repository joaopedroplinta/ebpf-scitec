// Programa de controle: configura a porta a bloquear no mapa, anexa o
// programa XDP a uma interface de rede escolhida explicitamente (nunca por
// padrão — ver aviso de segurança no README) e imprime o total de pacotes
// descartados a cada segundo.
//
// Usa modo XDP genérico (XDP_FLAGS_SKB_MODE) em vez do modo nativo/driver:
// funciona em qualquer interface (incluindo veth/dummy usadas para teste),
// às custas de parte do ganho de desempenho descrito na seção 4.5 — para
// uma NIC física com driver compatível, o modo nativo seria a escolha de
// produção.
#include <net/if.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <linux/if_link.h>

static volatile sig_atomic_t parar;
static int ifindex_global = -1;

static void parar_ao_receber_sinal(int sig)
{
    (void)sig;
    parar = 1;
}

int main(int argc, char **argv)
{
    struct bpf_object *obj;
    struct bpf_program *prog;
    int fd_config, fd_contador, prog_fd;
    __u32 chave = 0;
    __u16 porta;

    if (argc != 3) {
        fprintf(stderr, "uso: %s <interface> <porta_a_bloquear>\n", argv[0]);
        fprintf(stderr, "exemplo: sudo %s veth-teste 8080\n", argv[0]);
        return 1;
    }

    ifindex_global = if_nametoindex(argv[1]);
    if (!ifindex_global) {
        fprintf(stderr, "interface '%s' nao encontrada\n", argv[1]);
        return 1;
    }
    porta = (__u16)atoi(argv[2]);

    signal(SIGINT, parar_ao_receber_sinal);

    obj = bpf_object__open_file("build/xdp_drop.bpf.o", NULL);
    if (!obj) {
        fprintf(stderr, "erro ao abrir build/xdp_drop.bpf.o\n");
        return 1;
    }

    if (bpf_object__load(obj)) {
        fprintf(stderr, "erro ao carregar/verificar o programa (rode como root?)\n");
        return 1;
    }

    prog = bpf_object__find_program_by_name(obj, "bloquear_porta");
    if (!prog) {
        fprintf(stderr, "programa 'bloquear_porta' nao encontrado no objeto\n");
        return 1;
    }

    fd_config = bpf_object__find_map_fd_by_name(obj, "porta_bloqueada");
    fd_contador = bpf_object__find_map_fd_by_name(obj, "pacotes_descartados");
    if (fd_config < 0 || fd_contador < 0) {
        fprintf(stderr, "mapas nao encontrados\n");
        return 1;
    }
    bpf_map_update_elem(fd_config, &chave, &porta, BPF_ANY);

    prog_fd = bpf_program__fd(prog);
    if (bpf_xdp_attach(ifindex_global, prog_fd, XDP_FLAGS_SKB_MODE, NULL) < 0) {
        fprintf(stderr, "erro ao anexar XDP em '%s'\n", argv[1]);
        return 1;
    }

    printf("Bloqueando pacotes TCP/UDP com porta de destino %u em '%s'. Ctrl+C para sair.\n",
           porta, argv[1]);
    while (!parar) {
        __u64 total = 0;

        bpf_map_lookup_elem(fd_contador, &chave, &total);
        printf("\rpacotes descartados: %llu", (unsigned long long)total);
        fflush(stdout);
        sleep(1);
    }
    printf("\n");

    bpf_xdp_attach(ifindex_global, -1, XDP_FLAGS_SKB_MODE, NULL);
    bpf_object__close(obj);
    return 0;
}
