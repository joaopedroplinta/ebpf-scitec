# Ambiente Docker

Dois serviços:

- **dev** — container privilegiado com `clang`, `llvm`, `libbpf-dev`,
  `bpftool` e `python3-bpfcc`, usado para compilar e carregar os exemplos
  eBPF. Precisa ser privilegiado porque os programas eBPF são carregados no
  kernel do host (containers compartilham o mesmo kernel — é por isso que um
  coletor eBPF consegue observar tráfego de outros containers sem nenhuma
  configuração especial, como descrito na seção 7.6 do esqueleto do curso).
  Monta `/lib/modules` e `/usr/src` do host (somente leitura): são os
  cabeçalhos de kernel que o **BCC** (exemplos `python/`) precisa para
  compilar — os exemplos em C com libbpf/CO-RE não precisam disso, só do BTF
  em `/sys/kernel/btf` (essa é justamente a diferença prática entre as duas
  abordagens, seção 3.5).
- **toy-server** — servidor HTTP de brinquedo (porta 8080) que gera tráfego
  TCP para os exemplos observarem.

## Uso

```bash
cd docker
docker compose up -d --build

# entra no container de desenvolvimento
docker compose exec dev bash

# dentro do container: gera o vmlinux.h uma vez por máquina/kernel
../scripts/gen-vmlinux.sh examples/04-tcp-monitor/include

# compila e roda a versão em C de um exemplo (ver o Makefile/README de cada pasta)
cd examples/04-tcp-monitor
make
sudo ./build/monitor

# ou roda a versão equivalente em Python + BCC (sem etapa de compilação manual)
sudo python3 python/monitor.py
```

Em outro terminal, gere tráfego contra o toy-server para o coletor ter o que
mostrar:

```bash
./scripts/gen-traffic.sh localhost 8080 100
```

## Encerrando

```bash
docker compose down
```
