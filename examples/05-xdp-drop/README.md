# 05 — Drop de pacotes via XDP (extra opcional, C+libbpf/CO-RE e Python+BCC)

Demonstra o hook mais próximo do hardware (seção 4.5): o programa roda no
driver da placa de rede, antes de o kernel alocar qualquer estrutura para o
pacote, e decide `XDP_PASS` ou `XDP_DROP` olhando só para os cabeçalhos
Ethernet/IP/TCP/UDP brutos.

## ⚠️ Segurança — leia antes de rodar

**Nunca anexe este programa à interface de rede principal de uma máquina
remota ou compartilhada.** Um `XDP_DROP` mal configurado pode cortar o
próprio acesso SSH ou tráfego de outros usuários da máquina. Use sempre um
par veth com a ponta remota em outro *network namespace* — se as duas pontas
ficarem no mesmo namespace, o kernel entrega o tráfego local por um atalho
que nunca passa pelo hook XDP, e o exemplo pareceria "não funcionar":

```bash
# cria o namespace isolado e o par veth
sudo ip netns add ns-teste
sudo ip link add veth-teste type veth peer name veth-teste-par
sudo ip link set veth-teste-par netns ns-teste

# configura o lado que fica no namespace atual (onde o XDP será anexado)
sudo ip addr add 10.250.0.1/24 dev veth-teste
sudo ip link set veth-teste up

# configura o lado isolado
sudo ip netns exec ns-teste ip addr add 10.250.0.2/24 dev veth-teste-par
sudo ip netns exec ns-teste ip link set veth-teste-par up
sudo ip netns exec ns-teste ip link set lo up

# ao terminar
sudo ip netns del ns-teste
sudo ip link del veth-teste
```

O programa também usa modo **genérico** (`XDP_FLAGS_SKB_MODE`), que funciona
em qualquer interface (inclusive veth) às custas de desempenho — em
produção, numa NIC física com driver compatível, o modo nativo é a escolha
correta (seção 4.5).

Existe uma versão equivalente em Python + BCC em `python/xdp_drop.py`, com o
mesmo parsing e as mesmas duas chaves de mapa (`porta_bloqueada`,
`pacotes_descartados`).

## Rodando

```bash
# C + libbpf/CO-RE
make
sudo ./build/xdp_drop veth-teste 9999

# ou Python + BCC, equivalente
sudo python3 python/xdp_drop.py veth-teste 9999
```

Em outro terminal, gere tráfego UDP a partir do namespace isolado contra a
porta bloqueada e observe o contador subir enquanto um listener do lado
`veth-teste` não recebe nada:

```bash
# terminal A: listener do lado onde o XDP está anexado
python3 -c "
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(('10.250.0.1', 9999))
print(s.recvfrom(1024))
"

# terminal B: envia a partir do namespace isolado
sudo ip netns exec ns-teste python3 -c "
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.sendto(b'pacote-teste', ('10.250.0.1', 9999))
"
```

Com o `xdp_drop` rodando, o listener não recebe nada e o contador de
`pacotes descartados` sobe. Pare o `xdp_drop` (ou mude a porta bloqueada) e
repita: agora o listener recebe o pacote normalmente.

`Ctrl+C` remove o programa da interface antes de sair.

## O que observar

- Todo o parsing de cabeçalhos é manual, com verificação explícita de limite
  (`if ((void *)(x + 1) > data_end) return XDP_PASS;`) antes de cada acesso.
  O verificador (seção 3.3) rejeita o carregamento se alguma leitura não
  puder ser provada segura estaticamente — é comum errar esses limites ao
  escrever XDP pela primeira vez e ver o carregamento falhar com uma
  mensagem do verificador.
- Compare com o exemplo `04-tcp-monitor`: lá, o kernel já tinha alocado
  `struct sock` e outras estruturas com informação de conexão; aqui, no XDP,
  só existem os bytes crus do pacote — por isso não há acesso a estado de
  conexão (seção 4.5, limitação do XDP frente ao TC).

> **Status de teste:** a versão em C foi compilada, carregada e validada
> funcionalmente neste repositório (par veth cruzando dois network
> namespaces, pacote UDP descartado e contador incrementando). A versão em
> Python não pôde ser validada da mesma forma no ambiente usado para montar
> este repositório pelo mesmo motivo descrito em `04-tcp-monitor/README.md`:
> os cabeçalhos `<linux/tcp.h>`/`<linux/udp.h>` que o BCC precisa para
> `struct tcphdr`/`struct udphdr` puxam `linux/bpf.h`, e o pacote de headers
> deste kernel específico tem constantes inconsistentes entre si. Em uma
> máquina com headers de kernel consistentes, o código segue o padrão comum
> de programas XDP em BCC e é esperado que funcione sem alterações — mas
> vale testar antes da apresentação.
