#!/usr/bin/env bash
# Importa uma imagem gerada por build-image.sh (Docker + repo já
# provisionados) como uma nova VM local -- pensado para laboratórios sem
# internet: só copia o .qcow2 (pendrive/rede) e importa, sem baixar nada
# nem rodar cloud-init de novo.
set -euo pipefail

SRC_IMAGE="${1:?Uso: ./import-vm.sh <caminho-para-ebpf-scitec.qcow2>}"
VM_NAME="${VM_NAME:-ebpf-scitec}"
VM_RAM_MB="${VM_RAM_MB:-4096}"
VM_VCPUS="${VM_VCPUS:-2}"
VM_OS_VARIANT="${VM_OS_VARIANT:-ubuntu24.04}"

WORKDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGES_DIR="$WORKDIR/images"
mkdir -p "$IMAGES_DIR"
DISK_IMAGE="$IMAGES_DIR/${VM_NAME}.qcow2"

for bin in virt-install virsh; do
    command -v "$bin" >/dev/null || {
        echo "Erro: '$bin' não encontrado. Ver README.md desta pasta para os pacotes necessários." >&2
        exit 1
    }
done

[ -f "$SRC_IMAGE" ] || { echo "Erro: imagem '$SRC_IMAGE' não encontrada." >&2; exit 1; }

if virsh dominfo "$VM_NAME" >/dev/null 2>&1; then
    echo "Erro: já existe uma VM chamada '$VM_NAME'. Rode ./destroy-vm.sh $VM_NAME primeiro." >&2
    exit 1
fi

echo "Copiando imagem para $DISK_IMAGE..."
cp "$SRC_IMAGE" "$DISK_IMAGE"

echo "Importando VM '$VM_NAME' ($VM_VCPUS vCPU, ${VM_RAM_MB}MB RAM)..."
virt-install \
    --name "$VM_NAME" \
    --memory "$VM_RAM_MB" \
    --vcpus "$VM_VCPUS" \
    --disk path="$DISK_IMAGE",format=qcow2 \
    --os-variant "$VM_OS_VARIANT" \
    --network network=default \
    --graphics none \
    --console pty,target_type=serial \
    --import \
    --noautoconsole

cat <<EOF

VM '$VM_NAME' importada e ligando. Como a imagem já vem com Docker e o
repositório clonados (gerados por build-image.sh), não há provisionamento
para esperar -- só o boot normal.

Achar o IP:      virsh domifaddr $VM_NAME
Entrar por SSH:  ssh <usuário>@<ip>   (usuário/senha definidos no build)
Acompanhar boot: virsh console $VM_NAME   (Ctrl+] para sair)

Dentro da VM:
    cd ~/ebpf-scitec/docker
    docker compose up -d --build
    docker compose exec dev bash
EOF
