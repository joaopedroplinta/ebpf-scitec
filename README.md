# Minicurso eBPF

Minicurso introdutório de eBPF (Extended Berkeley Packet Filter), preparado para
apresentação em equipe. Cobre fundamentos teóricos (máquina virtual, verifier,
JIT, CO-RE, hooks) e uma prática guiada que replica, em versão simplificada, o
experimento do TCC que fundamenta o curso: um coletor eBPF observando tráfego
TCP de uma aplicação rodando em container Docker, sem passar pelo `/proc`.

## Conteúdo do repositório

- [`docs/Esqueleto-CursoEbpf.md`](./docs/Esqueleto-CursoEbpf.md) — roteiro teórico completo do minicurso (introdução, história do BPF, fundamentos, hooks, comparação com `/proc`/Sysstat/Prometheus, e o passo a passo do programa prático).
- [`docs/SciTec-eBPF.pdf`](./docs/SciTec-eBPF.pdf) — slides de apoio.

Estrutura do repositório:

```
minicurso-ebpf/
├── docs/                    material teórico e slides de apoio
├── docker/                  dev container (clang/llvm/libbpf/bpftool) + toy-server para gerar tráfego TCP
├── examples/
│   ├── 00-bcc-warmup/       Python + BCC — aquecimento (contador de execve)
│   ├── 01-hello-tracepoint/ C + libbpf/CO-RE — hello world com tracepoint
│   ├── 02-kprobe-counter/   C + libbpf — contador simples via kprobe
│   ├── 03-histogram-map/    C + libbpf — histograma usando BPF map
│   ├── 04-tcp-monitor/      C + libbpf — exemplo principal: bytes por porta TCP (kprobes em tcp_sendmsg/tcp_cleanup_rbuf)
│   └── 05-xdp-drop/         C + libbpf — extra opcional: drop de pacotes via XDP
└── scripts/                 setup de dependências, geração de vmlinux.h, checagem de ambiente, geração de tráfego
```

Cada pasta em `examples/` tem seu próprio `README.md` explicando o que o
exemplo demonstra, como rodar e o que observar — a progressão foi desenhada
para ser seguida em ordem (00 → 05), cada uma introduzindo um conceito novo
sobre a anterior. Todos os exemplos de `01` a `05` foram compilados e
testados em container (compilação, carregamento, verificador e leitura dos
mapas).

## Requisitos

- Kernel Linux 5.4+ com `CONFIG_DEBUG_INFO_BTF=y` (checar `/sys/kernel/btf/vmlinux`)
- Docker
- Privilégios de root (ou `CAP_BPF`/`CAP_SYS_ADMIN`) para carregar programas eBPF

Rode `scripts/check-env.sh` para validar o ambiente automaticamente.

## Início rápido (via Docker)

```bash
cd docker
docker compose up -d --build
docker compose exec dev bash
```

Ver [`docker/README.md`](./docker/README.md) para o fluxo completo (gerar
`vmlinux.h`, compilar e rodar os exemplos, gerar tráfego contra o
`toy-server`).

Quem preferir instalar as dependências direto no host (sem Docker) pode usar
`scripts/setup.sh` (equivalente à seção 6.2 do esqueleto do curso).

## Status

✅ Material teórico, ambiente Docker e os seis exemplos progressivos
prontos e testados. Próximos passos ficam a critério da equipe (ex: slides
finais, ensaio da apresentação).
