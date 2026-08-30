# Ambiente via libvirt (para máquinas sem Docker)

Alternativa ao [`docker/`](../docker/) para máquinas onde não é possível
instalar/rodar Docker diretamente (ex: laboratórios da faculdade, sem
permissão de root no host físico). A ideia: usar `libvirt` + QEMU/KVM --
normalmente já disponíveis nesses laboratórios para disciplinas de sistemas
-- para criar uma VM onde a equipe *tem* root, e rodar o Docker de sempre
*dentro* dela.

```
Host físico (faculdade)
  sem permissão/suporte a Docker
└── libvirt + QEMU/KVM
    gerencia a VM
    └── VM (kernel Linux próprio)
        BTF habilitado, root livre
        └── Docker (instalado dentro da VM)
            roda igual a um host normal
            ├── dev container      (clang/llvm/libbpf/bpftool)
            └── toy-server         (gera tráfego TCP)
```

Como a VM tem kernel próprio (não é o kernel "estranho" do host físico), o
Docker dentro dela funciona como em qualquer máquina normal, e o `docker/`
deste repositório roda sem nenhuma alteração -- inclusive é esperado que os
exemplos `04` e `05` em Python/BCC, que não puderam ser validados no
ambiente onde este repo foi montado, funcionem aqui, já que
`linux-headers-$(uname -r)` bate exatamente com o kernel que a própria VM
executa.

## Pré-requisitos (no host físico)

Pacotes: `qemu-kvm`, `libvirt-daemon-system`, `virtinst`, `cloud-image-utils`
(ou `genisoimage`), `openssl`. Em Debian/Ubuntu:

```bash
sudo apt-get install qemu-kvm libvirt-daemon-system virtinst cloud-image-utils
sudo usermod -aG libvirt,kvm "$USER"   # relogar depois
```

Verificar suporte a virtualização por hardware: `/dev/kvm` deve existir
(`ls /dev/kvm`). Sem isso o `virt-install` ainda funciona, mas em modo de
emulação por software (bem mais lento).

## Uso

```bash
cd libvirt
./create-vm.sh
```

Variáveis de ambiente opcionais (valores padrão entre parênteses):
`VM_NAME` (`ebpf-scitec`), `VM_RAM_MB` (`4096`), `VM_VCPUS` (`2`),
`VM_DISK_GB` (`20`), `VM_USER` (`ebpf`), `VM_PASSWORD` (`ebpf`),
`VM_OS_VARIANT` (`ubuntu24.04` -- trocar para `ubuntu22.04` ou `generic` se
o `osinfo-db` do host for mais antigo e não reconhecer o valor padrão).

O script baixa a imagem cloud do Ubuntu 24.04 (uma vez, fica em cache em
`libvirt/images/`), cria a VM via `virt-install` e injeta, via cloud-init:
usuário com sudo, sua chave SSH pública (`~/.ssh/id_ed25519.pub` ou
`id_rsa.pub`, se existir -- senão só login por senha), Docker, e um
`git clone` deste repositório em `~/ebpf-scitec`.

Acompanhar o provisionamento (leva alguns minutos no primeiro boot):

```bash
virsh console ebpf-scitec       # Ctrl+] para sair
```

Depois de pronta:

```bash
virsh domifaddr ebpf-scitec     # descobrir o IP
ssh ebpf@<ip>                   # senha padrão: ebpf
```

Dentro da VM, seguir exatamente o fluxo de [`../docker/README.md`](../docker/README.md):

```bash
cd ~/ebpf-scitec/docker
docker compose up -d --build
docker compose exec dev bash
```

## Removendo a VM

```bash
./destroy-vm.sh ebpf-scitec
```

Apaga o domínio libvirt e os discos (`libvirt/images/ebpf-scitec.qcow2` e a
ISO de seed). A imagem base do Ubuntu (`images/ubuntu-24.04-*.img`) não é
removida, para não precisar baixar de novo na próxima VM.
