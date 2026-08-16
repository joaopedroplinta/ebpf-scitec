// Idêntico em estrutura ao loader do exemplo 01 — o ciclo libbpf (abrir,
// carregar, anexar, ler o mapa) é o mesmo independentemente do tipo de hook.
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

int main(void)
{
    struct bpf_object *obj;
    struct bpf_program *prog;
    struct bpf_link *link;
    int map_fd;
    __u32 chave = 0;
    __u64 total;

    signal(SIGINT, parar_ao_receber_sinal);

    obj = bpf_object__open_file("build/counter.bpf.o", NULL);
    if (!obj) {
        fprintf(stderr, "erro ao abrir build/counter.bpf.o\n");
        return 1;
    }

    if (bpf_object__load(obj)) {
        fprintf(stderr, "erro ao carregar/verificar o programa (rode como root?)\n");
        return 1;
    }

    prog = bpf_object__find_program_by_name(obj, "ao_enviar");
    if (!prog) {
        fprintf(stderr, "programa 'ao_enviar' nao encontrado no objeto\n");
        return 1;
    }

    link = bpf_program__attach(prog);
    if (!link) {
        fprintf(stderr, "erro ao anexar a kprobe/tcp_sendmsg\n");
        return 1;
    }

    map_fd = bpf_object__find_map_fd_by_name(obj, "bytes_totais");
    if (map_fd < 0) {
        fprintf(stderr, "mapa 'bytes_totais' nao encontrado\n");
        return 1;
    }

    printf("Contando bytes enviados via TCP em todo o sistema... Ctrl+C para sair.\n");
    while (!parar) {
        if (bpf_map_lookup_elem(map_fd, &chave, &total) == 0)
            printf("\rbytes enviados (tcp_sendmsg) desde o inicio: %llu", (unsigned long long)total);
        fflush(stdout);
        sleep(1);
    }
    printf("\n");

    bpf_link__destroy(link);
    bpf_object__close(obj);
    return 0;
}
