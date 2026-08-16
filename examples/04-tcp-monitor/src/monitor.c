// Programa de controle: carrega monitor.bpf.o, anexa as duas kprobes e
// imprime periodicamente uma tabela porta -> bytes enviados/recebidos,
// lendo diretamente dos mapas eBPF (sem polling de /proc, seção 5.3.1).
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

static volatile sig_atomic_t parar;

static void parar_ao_receber_sinal(int sig)
{
    (void)sig;
    parar = 1;
}

static void imprimir_tabela(int fd_enviados, int fd_recebidos)
{
    __u16 chave_atual, proxima_chave;
    __u64 enviados, recebidos;
    int tem_chave_atual = 0;

    printf("\033[2J\033[H");
    printf("%-8s %14s %14s\n", "porta", "enviados(B)", "recebidos(B)");
    printf("------------------------------------------\n");

    while (bpf_map_get_next_key(fd_enviados, tem_chave_atual ? &chave_atual : NULL,
                                 &proxima_chave) == 0) {
        chave_atual = proxima_chave;
        tem_chave_atual = 1;

        enviados = 0;
        recebidos = 0;
        bpf_map_lookup_elem(fd_enviados, &chave_atual, &enviados);
        bpf_map_lookup_elem(fd_recebidos, &chave_atual, &recebidos);

        printf("%-8u %14llu %14llu\n", chave_atual,
               (unsigned long long)enviados, (unsigned long long)recebidos);
    }

    printf("\n(Ctrl+C para sair)\n");
}

int main(void)
{
    struct bpf_object *obj;
    struct bpf_program *prog_send, *prog_recv;
    struct bpf_link *link_send, *link_recv;
    int fd_enviados, fd_recebidos;

    signal(SIGINT, parar_ao_receber_sinal);

    obj = bpf_object__open_file("build/monitor.bpf.o", NULL);
    if (!obj) {
        fprintf(stderr, "erro ao abrir build/monitor.bpf.o\n");
        return 1;
    }

    if (bpf_object__load(obj)) {
        fprintf(stderr, "erro ao carregar/verificar o programa (rode como root?)\n");
        return 1;
    }

    prog_send = bpf_object__find_program_by_name(obj, "trace_tcp_sendmsg");
    prog_recv = bpf_object__find_program_by_name(obj, "trace_tcp_cleanup_rbuf");
    if (!prog_send || !prog_recv) {
        fprintf(stderr, "programas nao encontrados no objeto\n");
        return 1;
    }

    link_send = bpf_program__attach(prog_send);
    link_recv = bpf_program__attach(prog_recv);
    if (!link_send || !link_recv) {
        fprintf(stderr, "erro ao anexar as kprobes\n");
        return 1;
    }

    fd_enviados = bpf_object__find_map_fd_by_name(obj, "bytes_enviados");
    fd_recebidos = bpf_object__find_map_fd_by_name(obj, "bytes_recebidos");
    if (fd_enviados < 0 || fd_recebidos < 0) {
        fprintf(stderr, "mapas nao encontrados\n");
        return 1;
    }

    while (!parar) {
        imprimir_tabela(fd_enviados, fd_recebidos);
        fflush(stdout);
        sleep(1);
    }
    printf("\n");

    bpf_link__destroy(link_send);
    bpf_link__destroy(link_recv);
    bpf_object__close(obj);
    return 0;
}
