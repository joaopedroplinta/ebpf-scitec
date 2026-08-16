# 03 — Histograma com BPF map

Mesma kprobe em `tcp_sendmsg` do exemplo `02-kprobe-counter`, mas em vez de
somar tudo em um único contador, distribui os tamanhos de envio em 32 baldes
(potências de 2) usando um `BPF_MAP_TYPE_ARRAY` de 32 posições. É o mesmo
princípio usado por ferramentas como `bpftrace` para visualizar distribuições
de latência ou tamanho (seção 4.6 — mapas eBPF como estrutura central de
comunicação kernel ↔ espaço do usuário).

## Rodando

```bash
make
sudo ./build/hist
```

Gere tráfego variado em outro terminal — por exemplo, misture requisições
pequenas ao `toy-server` (`curl`) com transferências maiores (`curl` de um
arquivo grande, ou `dd` para um socket) — e observe as barras se
redistribuírem a cada atualização (2s).

## O que observar

- O cálculo do bucket (`log2_aprox`) roda **dentro do kernel**, a cada
  chamada de `tcp_sendmsg`. O espaço do usuário só lê os 32 contadores já
  agregados — nenhum dado bruto por pacote cruza a fronteira kernel/usuário,
  o ponto central da seção 5.3.1 (eliminação do polling e das cópias
  desnecessárias).
- O laço `for` em `log2_aprox` usa `#pragma unroll`: o verificador do eBPF
  precisa provar que todo programa termina (seção 3.3), e laços com limite
  fixo e conhecido em tempo de compilação são a forma mais simples de
  satisfazer essa exigência.
