#!/usr/bin/env bash
# Verifica se o ambiente atende aos requisitos da seção 6.1 do esqueleto do
# curso (docs/Esqueleto-CursoEbpf.md) para rodar os exemplos eBPF.
set -uo pipefail

ok=0
fail=0

check() {
    local desc="$1" cond="$2"
    if eval "$cond"; then
        echo "[OK]   $desc"
        ok=$((ok + 1))
    else
        echo "[FAIL] $desc"
        fail=$((fail + 1))
    fi
}

kernel_version="$(uname -r | cut -d- -f1)"

check "kernel >= 5.4 (atual: $(uname -r))" \
    '[ "$(printf "%s\n%s" "5.4" "$kernel_version" | sort -V | head -1)" = "5.4" ]'
check "/sys/kernel/btf/vmlinux presente (suporte a BTF/CO-RE)" \
    '[ -e /sys/kernel/btf/vmlinux ]'
check "clang instalado" 'command -v clang >/dev/null'
check "bpftool instalado" 'command -v bpftool >/dev/null'
check "docker instalado" 'command -v docker >/dev/null'
check "rodando como root (necessário para carregar programas eBPF)" \
    '[ "$(id -u)" = "0" ]'

echo
echo "$ok verificações OK, $fail falharam."
[ "$fail" -eq 0 ]
