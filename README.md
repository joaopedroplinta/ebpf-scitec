# Minicurso eBPF

Minicurso introdutório de eBPF (Extended Berkeley Packet Filter), preparado para
apresentação em equipe. Cobre fundamentos teóricos (máquina virtual, verifier,
JIT, CO-RE, hooks) e uma prática guiada que replica, em versão simplificada, o
experimento do TCC que fundamenta o curso: um coletor eBPF observando tráfego
TCP de uma aplicação rodando em container Docker, sem passar pelo `/proc`.

## Conteúdo do repositório

- [`docs/Esqueleto-CursoEbpf.md`](./docs/Esqueleto-CursoEbpf.md) — roteiro teórico completo do minicurso (introdução, história do BPF, fundamentos, hooks, comparação com `/proc`/Sysstat/Prometheus, e o passo a passo do programa prático).
- [`docs/SciTec-eBPF.pdf`](./docs/SciTec-eBPF.pdf) — slides de apoio.

O restante da estrutura (exemplos de código, Docker, scripts) está sendo
construído incrementalmente. Estrutura planejada:

```
minicurso-ebpf/
├── docs/                  material teórico e slides de apoio
├── docker/                dev container (clang/llvm/libbpf/bpftool) + app de brinquedo para gerar tráfego TCP
├── examples/
│   ├── 00-bcc-warmup/      Python + BCC — aquecimento (contador de execve)
│   ├── 01-hello-tracepoint/ C + libbpf/CO-RE — hello world com tracepoint
│   ├── 02-kprobe-counter/  C + libbpf — contador simples via kprobe
│   ├── 03-histogram-map/   C + libbpf — histograma usando BPF map
│   ├── 04-tcp-monitor/     C + libbpf — exemplo principal: bytes por porta TCP (kprobes em tcp_sendmsg/tcp_cleanup_rbuf)
│   └── 05-xdp-drop/        C + libbpf — extra opcional: drop de pacotes via XDP
└── scripts/                setup de dependências, geração de vmlinux.h, checagem de ambiente
```

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

🚧 Em construção — ambiente Docker e scripts de setup prontos. Próximo passo:
os exemplos progressivos em `examples/`.
