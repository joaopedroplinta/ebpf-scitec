#!/usr/bin/env bash
# Cria, via libvirt/QEMU-KVM, uma VM com Docker já instalado -- para rodar o
# ambiente do minicurso (docker/) em máquinas onde Docker não pode ser usado
# diretamente no host físico (ex: laboratórios da faculdade). A VM tem kernel
# próprio e root livre; o Docker roda dentro dela normalmente. Ver README.md
# desta pasta para o desenho completo.
set -euo pipefail

VM_NAME="${VM_NAME:-ebpf-scitec}"
VM_RAM_MB="${VM_RAM_MB:-4096}"
VM_VCPUS="${VM_VCPUS:-2}"
VM_DISK_GB="${VM_DISK_GB:-20}"
VM_USER="${VM_USER:-ebpf}"
VM_PASSWORD="${VM_PASSWORD:-ebpf}"
VM_OS_VARIANT="${VM_OS_VARIANT:-ubuntu24.04}"

WORKDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$WORKDIR/build"
IMAGES_DIR="$WORKDIR/images"
mkdir -p "$BUILD_DIR" "$IMAGES_DIR"

BASE_IMAGE_URL="https://cloud-images.ubuntu.com/releases/24.04/release/ubuntu-24.04-server-cloudimg-amd64.img"
BASE_IMAGE="$IMAGES_DIR/ubuntu-24.04-server-cloudimg-amd64.img"
DISK_IMAGE="$IMAGES_DIR/${VM_NAME}.qcow2"
SEED_IMAGE="$BUILD_DIR/${VM_NAME}-seed.iso"

for bin in virt-install virsh qemu-img openssl python3; do
    command -v "$bin" >/dev/null || {
        echo "Erro: '$bin' não encontrado. Ver README.md desta pasta para os pacotes necessários." >&2
        exit 1
    }
done

if virsh dominfo "$VM_NAME" >/dev/null 2>&1; then
    echo "Erro: já existe uma VM chamada '$VM_NAME'. Rode ./destroy-vm.sh $VM_NAME primeiro." >&2
    exit 1
fi

if [ ! -e "$BASE_IMAGE" ]; then
    echo "Baixando imagem base do Ubuntu 24.04 (uma vez só, fica em cache em $IMAGES_DIR)..."
    curl -L -o "$BASE_IMAGE.tmp" "$BASE_IMAGE_URL"
    mv "$BASE_IMAGE.tmp" "$BASE_IMAGE"
fi

echo "Criando disco de ${VM_DISK_GB}G a partir da imagem base..."
qemu-img create -f qcow2 -F qcow2 -b "$BASE_IMAGE" "$DISK_IMAGE" "${VM_DISK_GB}G" >/dev/null

SSH_PUBKEY=""
for key in "$HOME/.ssh/id_ed25519.pub" "$HOME/.ssh/id_rsa.pub"; do
    if [ -f "$key" ]; then
        SSH_PUBKEY="$(cat "$key")"
        break
    fi
done
if [ -z "$SSH_PUBKEY" ]; then
    echo "Aviso: nenhuma chave SSH pública encontrada em ~/.ssh -- login por SSH ficará só por senha."
fi

PASSWORD_HASH="$(openssl passwd -6 "$VM_PASSWORD")"

python3 - "$WORKDIR/cloud-init/user-data.tmpl" "$BUILD_DIR/user-data" \
    "$VM_USER" "$PASSWORD_HASH" "$SSH_PUBKEY" <<'PYEOF'
import sys
tmpl_path, out_path, user, password_hash, ssh_pubkey = sys.argv[1:6]
text = open(tmpl_path).read()
text = text.replace("__VM_USER__", user)
text = text.replace("__PASSWORD_HASH__", password_hash)
text = text.replace("__SSH_PUBKEY__", ssh_pubkey)
open(out_path, "w").write(text)
PYEOF
cp "$WORKDIR/cloud-init/meta-data" "$BUILD_DIR/meta-data"

if command -v cloud-localds >/dev/null; then
    cloud-localds "$SEED_IMAGE" "$BUILD_DIR/user-data" "$BUILD_DIR/meta-data"
elif command -v genisoimage >/dev/null; then
    genisoimage -output "$SEED_IMAGE" -volid cidata -joliet -rock \
        "$BUILD_DIR/user-data" "$BUILD_DIR/meta-data" >/dev/null
else
    echo "Erro: precisa de 'cloud-localds' (pacote cloud-image-utils) ou 'genisoimage'." >&2
    exit 1
fi

echo "Criando VM '$VM_NAME' ($VM_VCPUS vCPU, ${VM_RAM_MB}MB RAM, ${VM_DISK_GB}G disco)..."
virt-install \
    --name "$VM_NAME" \
    --memory "$VM_RAM_MB" \
    --vcpus "$VM_VCPUS" \
    --disk path="$DISK_IMAGE",format=qcow2 \
    --disk path="$SEED_IMAGE",device=cdrom \
    --os-variant "$VM_OS_VARIANT" \
    --network network=default \
    --graphics none \
    --console pty,target_type=serial \
    --import \
    --noautoconsole

cat <<EOF

VM '$VM_NAME' criada. O provisionamento (cloud-init: instala Docker, clona o
repositório) roda no primeiro boot e leva alguns minutos.

Usuário: $VM_USER   Senha: $VM_PASSWORD

Acompanhar o boot/provisionamento:  virsh console $VM_NAME   (Ctrl+] para sair)
Achar o IP depois de pronta:        virsh domifaddr $VM_NAME
Entrar por SSH:                     ssh $VM_USER@<ip>

Dentro da VM, o repositório já está clonado em ~/ebpf-scitec (ver /etc/motd).
EOF
