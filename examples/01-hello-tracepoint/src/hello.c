// Programa de controle (espaço usuário): carrega hello.bpf.o no kernel via
// libbpf, anexa ao tracepoint e lê o mapa "contador" a cada segundo. Ver
// seções 7.3 a 7.5 do esqueleto do curso para o fluxo completo.
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

    obj = bpf_object__open_file("build/hello.bpf.o", NULL);
    if (!obj) {
        fprintf(stderr, "erro ao abrir build/hello.bpf.o\n");
        return 1;
    }

    if (bpf_object__load(obj)) {
        fprintf(stderr, "erro ao carregar/verificar o programa (rode como root?)\n");
        return 1;
    }

    prog = bpf_object__find_program_by_name(obj, "rastrear_execve");
    if (!prog) {
        fprintf(stderr, "programa 'rastrear_execve' nao encontrado no objeto\n");
        return 1;
    }

    link = bpf_program__attach(prog);
    if (!link) {
        fprintf(stderr, "erro ao anexar ao tracepoint\n");
        return 1;
    }

    map_fd = bpf_object__find_map_fd_by_name(obj, "contador");
    if (map_fd < 0) {
        fprintf(stderr, "mapa 'contador' nao encontrado\n");
        return 1;
    }

    printf("Rastreando execve() em todo o sistema... Ctrl+C para sair.\n");
    while (!parar) {
        if (bpf_map_lookup_elem(map_fd, &chave, &total) == 0)
            printf("\rtotal de execve() desde o inicio: %llu", (unsigned long long)total);
        fflush(stdout);
        sleep(1);
    }
    printf("\n");

    bpf_link__destroy(link);
    bpf_object__close(obj);
    return 0;
}
