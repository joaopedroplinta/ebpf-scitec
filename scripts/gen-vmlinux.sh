#!/usr/bin/env bash
# Gera include/vmlinux.h a partir do BTF do kernel local, usado pelos
# exemplos em C+libbpf/CO-RE (ver seção 3.5 e 6.3 do esqueleto do curso).
# Precisa rodar em uma máquina (ou container) que enxergue
# /sys/kernel/btf/vmlinux do host.
set -euo pipefail

OUT_DIR="${1:-include}"
mkdir -p "$OUT_DIR"

if [ ! -e /sys/kernel/btf/vmlinux ]; then
    echo "Erro: /sys/kernel/btf/vmlinux não encontrado." >&2
    echo "O kernel precisa ser compilado com CONFIG_DEBUG_INFO_BTF=y (Ubuntu 20.04+ já vem assim)." >&2
    exit 1
fi

if ! command -v bpftool >/dev/null; then
    echo "Erro: bpftool não encontrado no PATH." >&2
    exit 1
fi

bpftool btf dump file /sys/kernel/btf/vmlinux format c > "$OUT_DIR/vmlinux.h"
echo "Gerado $OUT_DIR/vmlinux.h ($(wc -l < "$OUT_DIR/vmlinux.h") linhas)."
