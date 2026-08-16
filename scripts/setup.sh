#!/usr/bin/env bash
# Instala as dependências para desenvolvimento eBPF diretamente no host
# (sem Docker). Ver seção 6.2 do esqueleto do curso (docs/Esqueleto-CursoEbpf.md).
set -euo pipefail

sudo apt-get update
sudo apt-get install -y \
    clang llvm libbpf-dev libelf-dev \
    linux-tools-common linux-tools-generic \
    python3 python3-pip python3-bpfcc \
    make git curl

# O pacote linux-tools-$(uname -r) nem sempre existe para kernels
# customizados/muito recentes; tenta instalar, mas não falha o script se
# não encontrar (o linux-tools-generic acima já cobre o bpftool na maioria
# dos casos).
sudo apt-get install -y "linux-tools-$(uname -r)" "linux-headers-$(uname -r)" || \
    echo "Aviso: linux-tools-$(uname -r) indisponível; seguindo com linux-tools-generic."

echo
echo "Dependências instaladas. Rode scripts/check-env.sh para validar o ambiente."
