#!/usr/bin/env bash
# Baixa do Google Drive a imagem gerada por build-image.sh -- útil em
# laboratórios sem pendrive/rede local à mão, mas com acesso ao Drive.
# Precisa que o arquivo esteja compartilhado como "Qualquer pessoa com o
# link" (modo leitor). Pegue o ID na URL de compartilhamento:
#   https://drive.google.com/file/d/ESTE_PEDACO_AQUI/view?usp=sharing
set -euo pipefail

FILE_ID="${1:?Uso: ./fetch-image.sh <ID-do-arquivo-no-drive> [destino.qcow2]}"
OUT="${2:-ebpf-scitec.qcow2}"

COOKIES="$(mktemp)"
trap 'rm -f "$COOKIES"' EXIT

echo "Baixando de drive.google.com (id=$FILE_ID)..."
curl -sc "$COOKIES" "https://drive.google.com/uc?export=download&id=${FILE_ID}" -o /dev/null
CONFIRM="$(awk '/download/ {print $NF}' "$COOKIES")"
[ -z "$CONFIRM" ] && CONFIRM="t"
curl -Lb "$COOKIES" "https://drive.google.com/uc?export=download&confirm=${CONFIRM}&id=${FILE_ID}" -o "$OUT"

if head -c 200 "$OUT" | grep -qi "<!DOCTYPE\|<html"; then
    echo "Erro: o Drive devolveu uma página HTML em vez do arquivo (link errado," >&2
    echo "sem permissão, ou o formato de confirmação do Google mudou)." >&2
    echo "Baixe manualmente pelo navegador e use o arquivo direto com ./import-vm.sh." >&2
    rm -f "$OUT"
    exit 1
fi

echo "Salvo em $OUT ($(du -h "$OUT" | cut -f1))"
echo "Agora rode: ./import-vm.sh $OUT"
