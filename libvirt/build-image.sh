#!/usr/bin/env bash
# Builda uma VM completa (via create-vm.sh, com Docker + repo já
# provisionados) e "achata" o disco numa imagem qcow2 autônoma em dist/ --
# pronta para copiar via pendrive/rede e importar em outras máquinas com
# import-vm.sh, sem precisar de internet nem reprovisionar em cada uma.
set -euo pipefail

VM_NAME="${VM_NAME:-ebpf-scitec-build}"
VM_USER="${VM_USER:-ebpf}"
SSH_TIMEOUT_S="${SSH_TIMEOUT_S:-600}"

WORKDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DIST_DIR="$WORKDIR/dist"
mkdir -p "$DIST_DIR"
OUTPUT_IMAGE="$DIST_DIR/ebpf-scitec.qcow2"
DISK_IMAGE="$WORKDIR/images/${VM_NAME}.qcow2"

HAVE_KEY=0
for key in "$HOME/.ssh/id_ed25519.pub" "$HOME/.ssh/id_rsa.pub"; do
    [ -f "$key" ] && HAVE_KEY=1
done
if [ "$HAVE_KEY" -eq 0 ]; then
    echo "Erro: build-image.sh precisa de uma chave SSH em ~/.ssh (id_ed25519 ou" >&2
    echo "id_rsa) para saber quando o cloud-init terminou, sem senha interativa." >&2
    echo "Gere uma com: ssh-keygen -t ed25519" >&2
    exit 1
fi

if virsh dominfo "$VM_NAME" >/dev/null 2>&1; then
    echo "Erro: já existe uma VM chamada '$VM_NAME'. Rode ./destroy-vm.sh $VM_NAME primeiro." >&2
    exit 1
fi

echo "==> Criando e provisionando a VM (create-vm.sh)..."
VM_NAME="$VM_NAME" VM_USER="$VM_USER" "$WORKDIR/create-vm.sh"

echo "==> Aguardando a VM receber um IP..."
IP=""
for _ in $(seq 1 60); do
    IP="$(virsh domifaddr "$VM_NAME" --source lease 2>/dev/null | awk '/ipv4/ {print $4}' | cut -d/ -f1)"
    [ -n "$IP" ] && break
    sleep 5
done
if [ -z "$IP" ]; then
    echo "Erro: a VM não recebeu IP a tempo. Veja 'virsh console $VM_NAME'." >&2
    exit 1
fi
echo "    IP: $IP"

SSH_OPTS=(-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=5)

echo "==> Aguardando cloud-init terminar (instala Docker, clona o repo)..."
DEADLINE=$((SECONDS + SSH_TIMEOUT_S))
until ssh "${SSH_OPTS[@]}" "$VM_USER@$IP" "cloud-init status --wait" 2>/dev/null; do
    if [ "$SECONDS" -ge "$DEADLINE" ]; then
        echo "Erro: cloud-init não terminou em ${SSH_TIMEOUT_S}s. Veja 'virsh console $VM_NAME'." >&2
        exit 1
    fi
    sleep 10
done

echo "==> Generalizando a config de rede (não pode depender do MAC desta VM --"
echo "    cada máquina que importar a imagem gera um domínio com MAC diferente)..."
ssh "${SSH_OPTS[@]}" "$VM_USER@$IP" "sudo tee /etc/netplan/50-cloud-init.yaml >/dev/null" <<'NETPLAN'
network:
  version: 2
  ethernets:
    all-eth:
      match:
        name: "en*"
      dhcp4: true
NETPLAN

echo "==> Desligando a VM..."
virsh shutdown "$VM_NAME"
for _ in $(seq 1 30); do
    LC_ALL=C virsh dominfo "$VM_NAME" 2>/dev/null | grep -q "shut off" && break
    sleep 5
done
if ! LC_ALL=C virsh dominfo "$VM_NAME" | grep -q "shut off"; then
    echo "VM não desligou a tempo, forçando..." >&2
    virsh destroy "$VM_NAME" 2>/dev/null || true
    sleep 2
fi
if ! LC_ALL=C virsh dominfo "$VM_NAME" | grep -q "shut off"; then
    echo "Erro: não foi possível desligar a VM antes de gerar a imagem." >&2
    exit 1
fi

echo "==> Achatando o disco numa imagem autônoma ($OUTPUT_IMAGE)..."
rm -f "$OUTPUT_IMAGE"
qemu-img convert -O qcow2 "$DISK_IMAGE" "$OUTPUT_IMAGE"

echo "==> Removendo a VM temporária (mantém a imagem final e a base do Ubuntu em cache)..."
virsh undefine "$VM_NAME" --remove-all-storage

cat <<EOF

Imagem pronta: $OUTPUT_IMAGE ($(du -h "$OUTPUT_IMAGE" | cut -f1))

Leve esse arquivo para o laboratório (pendrive, rede, etc.) e rode, em cada
máquina de destino:

    ./import-vm.sh $OUTPUT_IMAGE

Usuário dentro da VM: $VM_USER   (senha definida na hora do build, padrão "ebpf")
EOF
