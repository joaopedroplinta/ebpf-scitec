#!/usr/bin/env bash
# Remove a VM criada por create-vm.sh (domínio libvirt + discos qcow2/seed).
set -euo pipefail

VM_NAME="${1:-ebpf-scitec}"

if ! virsh dominfo "$VM_NAME" >/dev/null 2>&1; then
    echo "Nenhuma VM chamada '$VM_NAME' encontrada."
    exit 0
fi

read -r -p "Isso vai destruir e apagar a VM '$VM_NAME' (disco incluso). Confirma? [y/N] " ans
case "$ans" in
    y|Y) ;;
    *) echo "Cancelado."; exit 0 ;;
esac

virsh destroy "$VM_NAME" 2>/dev/null || true
virsh undefine "$VM_NAME" --remove-all-storage
echo "VM '$VM_NAME' removida."
