// Lê as 32 posições do mapa "histograma" e desenha barras ASCII
// proporcionais à maior contagem, atualizando a cada 2 segundos.
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#define NUM_BUCKETS 32
#define LARGURA_MAX_BARRA 40

static volatile sig_atomic_t parar;

static void parar_ao_receber_sinal(int sig)
{
    (void)sig;
    parar = 1;
}

static void imprimir_histograma(int map_fd)
{
    __u64 valores[NUM_BUCKETS] = {0};
    __u64 maior = 1;

    for (__u32 i = 0; i < NUM_BUCKETS; i++)
        bpf_map_lookup_elem(map_fd, &i, &valores[i]);

    for (__u32 i = 0; i < NUM_BUCKETS; i++)
        if (valores[i] > maior)
            maior = valores[i];

    printf("\033[2J\033[H"); // limpa o terminal
    printf("tamanho de tcp_sendmsg() em bytes (potencias de 2) -- Ctrl+C para sair\n\n");
    for (__u32 i = 0; i < NUM_BUCKETS; i++) {
        if (valores[i] == 0)
            continue;
        int largura = (int)((valores[i] * LARGURA_MAX_BARRA) / maior);
        printf("%6u .. %-6u | ", (i == 0) ? 0 : (1u << (i - 1)) + 1, 1u << i);
        for (int j = 0; j < largura; j++)
            putchar('#');
        printf(" %llu\n", (unsigned long long)valores[i]);
    }
}

int main(void)
{
    struct bpf_object *obj;
    struct bpf_program *prog;
    struct bpf_link *link;
    int map_fd;

    signal(SIGINT, parar_ao_receber_sinal);

    obj = bpf_object__open_file("build/hist.bpf.o", NULL);
    if (!obj) {
        fprintf(stderr, "erro ao abrir build/hist.bpf.o\n");
        return 1;
    }

    if (bpf_object__load(obj)) {
        fprintf(stderr, "erro ao carregar/verificar o programa (rode como root?)\n");
        return 1;
    }

    prog = bpf_object__find_program_by_name(obj, "rastrear_tamanho");
    if (!prog) {
        fprintf(stderr, "programa 'rastrear_tamanho' nao encontrado no objeto\n");
        return 1;
    }

    link = bpf_program__attach(prog);
    if (!link) {
        fprintf(stderr, "erro ao anexar a kprobe/tcp_sendmsg\n");
        return 1;
    }

    map_fd = bpf_object__find_map_fd_by_name(obj, "histograma");
    if (map_fd < 0) {
        fprintf(stderr, "mapa 'histograma' nao encontrado\n");
        return 1;
    }

    while (!parar) {
        imprimir_histograma(map_fd);
        fflush(stdout);
        sleep(2);
    }
    printf("\n");

    bpf_link__destroy(link);
    bpf_object__close(obj);
    return 0;
}
