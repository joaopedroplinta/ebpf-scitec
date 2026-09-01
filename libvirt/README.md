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

Existem dois jeitos de usar isso -- **recomendação do orientador é o
Fluxo B** para os laboratórios da faculdade: gerar a imagem uma vez e
importar ela pronta em cada máquina, em vez de provisionar (baixar imagem,
instalar Docker, clonar o repo) do zero em cada uma.

## Pré-requisitos (em qualquer máquina, host físico)

Pacotes: `qemu-kvm`, `libvirt-daemon-system`, `virtinst`, `cloud-image-utils`
(ou `genisoimage`), `openssl`. Em Debian/Ubuntu:

```bash
sudo apt-get install qemu-kvm libvirt-daemon-system virtinst cloud-image-utils
sudo usermod -aG libvirt,kvm "$USER"   # relogar depois
```

Verificar suporte a virtualização por hardware: `/dev/kvm` deve existir
(`ls /dev/kvm`). Sem isso o `virt-install` ainda funciona, mas em modo de
emulação por software (bem mais lento).

## Fluxo A -- criar e provisionar direto numa máquina

Bom para testar rápido numa única máquina (a sua, por exemplo). Baixa a
imagem cloud do Ubuntu, cria a VM e provisiona tudo (Docker, git clone) via
cloud-init -- precisa de internet na hora de rodar.

```bash
cd libvirt
./create-vm.sh
```

Variáveis de ambiente opcionais (valores padrão entre parênteses):
`VM_NAME` (`ebpf-scitec`), `VM_RAM_MB` (`4096`), `VM_VCPUS` (`2`),
`VM_DISK_GB` (`20`), `VM_USER` (`ebpf`), `VM_PASSWORD` (`ebpf`),
`VM_OS_VARIANT` (`ubuntu24.04` -- trocar para `ubuntu22.04` ou `generic` se
o `osinfo-db` do host for mais antigo e não reconhecer o valor padrão).

O script injeta, via cloud-init: usuário com sudo, sua chave SSH pública
(`~/.ssh/id_ed25519.pub` ou `id_rsa.pub`, se existir -- senão só login por
senha), Docker, e um `git clone` deste repositório em `~/ebpf-scitec`.

Acompanhar o provisionamento (leva alguns minutos no primeiro boot):

```bash
virsh console ebpf-scitec       # Ctrl+] para sair
```

## Fluxo B -- gerar a imagem uma vez, importar em cada máquina do laboratório

Recomendado quando os laboratórios não têm internet suficiente (ou tempo,
com várias máquinas) para provisionar do zero em cada uma. A ideia: builda
a VM completa (Docker + repo já dentro) **uma vez**, na sua máquina, e essa
imagem pronta (um único arquivo `.qcow2`) é levada via pendrive/rede e
importada em cada máquina do laboratório sem baixar nada nem rodar
cloud-init de novo.

**1. Gerar a imagem (na sua máquina, com internet):**

```bash
cd libvirt
./build-image.sh
```

Isso roda o `create-vm.sh` por baixo, espera o cloud-init terminar (via
SSH -- por isso precisa de uma chave em `~/.ssh/id_ed25519` ou `id_rsa`),
desliga a VM, achata o disco (remove a dependência da imagem base) e gera
`libvirt/dist/ebpf-scitec.qcow2`.

**2. Levar o arquivo pro laboratório.** Em labs grandes (30+ PCs), copiar por
pendrive máquina a máquina não escala. Opções:

- **Google Drive** (se o laboratório tem acesso, mesmo sem internet livre
  em geral): subir `ebpf-scitec.qcow2` e compartilhar como "Qualquer pessoa
  com o link". Pegar o ID na URL (`.../file/d/ESTE_PEDACO/view`) e, em cada
  PC:
  ```bash
  cd libvirt
  ./fetch-image.sh <ID-do-arquivo> ebpf-scitec.qcow2
  ```
  Se o Google mudar o formato da página de confirmação e o script falhar,
  baixar manualmente pelo navegador funciona igual (clicar em "Fazer
  download mesmo assim" no aviso de arquivo grande).
- **Rede local do laboratório** (servidor HTTP simples numa máquina,
  `python3 -m http.server`, e as outras baixam com `curl`/`wget`) --
  melhor opção se não houver acesso ao Drive.
- **Pendrive** -- ok pra poucas máquinas; atenção ao limite de 4GB do
  FAT32 (usar exFAT se a imagem passar disso).

**3. Importar em cada máquina do laboratório:**

```bash
cd libvirt
./import-vm.sh /caminho/para/ebpf-scitec.qcow2
```

Sem downloads, sem cloud-init -- só copia o disco para `libvirt/images/` e
sobe a VM com `virt-install --import`. Variáveis opcionais: `VM_NAME`,
`VM_RAM_MB`, `VM_VCPUS`, `VM_OS_VARIANT` (mesmos defaults do Fluxo A).

## Depois de pronta (qualquer um dos dois fluxos)

```bash
virsh domifaddr ebpf-scitec     # descobrir o IP
ssh ebpf@<ip>                   # usuário/senha definidos no build (padrão ebpf/ebpf)
```

Dentro da VM, seguir exatamente o fluxo de [`../docker/README.md`](../docker/README.md):

```bash
cd ~/ebpf-scitec/docker
docker compose up -d --build
docker compose exec dev bash
```

## Removendo uma VM

```bash
./destroy-vm.sh ebpf-scitec
```

Apaga o domínio libvirt e os discos (`libvirt/images/ebpf-scitec.qcow2` e a
ISO de seed, se houver). A imagem base do Ubuntu (`images/ubuntu-24.04-*.img`)
e a imagem gerada em `dist/` não são removidas.
