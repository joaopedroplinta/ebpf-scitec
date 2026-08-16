# 01 — Hello tracepoint (C+libbpf/CO-RE e Python+BCC)

O mesmo contador de `execve()` do aquecimento (`00-bcc-warmup`), agora
guardado em um **mapa eBPF** de verdade (`BPF_MAP_TYPE_ARRAY`) em vez do
`bpf_trace_printk`/`trace_pipe` usado ali — implementado nas duas stacks do
curso lado a lado, para comparação direta:

```
src/
  hello.bpf.c   programa eBPF (espaço kernel) — SEC("tracepoint/...")
  hello.c       programa de controle (espaço usuário) — libbpf
python/
  hello_execve.py   mesma lógica, via BCC (TRACEPOINT_PROBE + BPF_ARRAY)
```

## Rodando (C + libbpf/CO-RE)

```bash
make            # gera include/vmlinux.h (uma vez) e compila hello.bpf.o + hello
sudo ./build/hello
```

Em outro terminal, rode qualquer comando (`ls`, `curl`) para ver o contador
subir. `Ctrl+C` para sair.

## Rodando (Python + BCC)

```bash
sudo python3 python/hello_execve.py
```

Mesmo comportamento, sem etapa de compilação manual — o BCC compila o texto
C embutido em tempo de execução. Ambas as versões deste exemplo foram
testadas de ponta a ponta no container `dev`.

## Comparando com o aquecimento em BCC

- Aqui a compilação (`clang -target bpf`) acontece uma única vez, antes da
  execução — no BCC, ela acontece a cada `python3 hello_execve.py`. Em
  produção, isso significa não depender de `clang`/cabeçalhos do kernel
  instalados na máquina alvo (seção 3.5, CO-RE).
- O dado sai do kernel por um **mapa eBPF** (`BPF_MAP_TYPE_ARRAY`), lido sob
  demanda pelo espaço do usuário — o mesmo mecanismo usado no exemplo
  principal (`04-tcp-monitor`) e descrito na seção 4.6. Isso substitui o
  `bpf_trace_printk`/`trace_pipe` usado no aquecimento, que não é adequado
  para produção.

## Erros comuns

- `erro ao carregar/verificar o programa`: rode com `sudo` — carregar
  programas eBPF exige `CAP_BPF`/`CAP_SYS_ADMIN`.
- `include/vmlinux.h` desatualizado após trocar de máquina/kernel: rode
  `make clean && make` para regerá-lo.
