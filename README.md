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
│   ├── 00-bcc-warmup/       Python + BCC — aquecimento (contador de execve via bpf_trace_printk)
│   ├── 01-hello-tracepoint/ hello world com tracepoint — C+libbpf/CO-RE e Python+BCC
│   ├── 02-kprobe-counter/   contador simples via kprobe — C+libbpf/CO-RE e Python+BCC
│   ├── 03-histogram-map/    histograma usando BPF map — C+libbpf/CO-RE e Python+BCC
│   ├── 04-tcp-monitor/      exemplo principal: bytes por porta TCP — C+libbpf/CO-RE e Python+BCC
│   └── 05-xdp-drop/         extra opcional: drop de pacotes via XDP — C+libbpf/CO-RE e Python+BCC
└── scripts/                 setup de dependências, geração de vmlinux.h, checagem de ambiente, geração de tráfego
```

Cada pasta em `examples/` tem seu próprio `README.md` explicando o que o
exemplo demonstra, como rodar e o que observar. De `01` a `05`, cada exemplo
existe nas duas stacks do curso lado a lado: `src/` (C + libbpf/CO-RE) e
`python/` (Python + BCC) — a progressão foi desenhada para ser seguida em
ordem, cada uma introduzindo um conceito novo sobre a anterior.

**Status de teste:** todas as versões em C (`01`–`05`) e as versões em
Python de `00`–`03` foram compiladas/carregadas e validadas de ponta a ponta
neste repositório. As versões em Python de `04` e `05` não puderam ser
validadas da mesma forma no ambiente usado para montar este repositório —
o pacote de headers do kernel deste host tem inconsistências internas que
impedem o BCC de compilar contra `<net/sock.h>`/`<linux/bpf.h>` (detalhes no
README de cada uma). O código segue os mesmos padrões usados por ferramentas
BCC reais e é esperado que funcione em uma máquina com headers consistentes,
mas vale testar antes da apresentação.

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
