# 03 — Histograma com BPF map (C+libbpf/CO-RE e Python+BCC)

Mesma kprobe em `tcp_sendmsg` do exemplo `02-kprobe-counter`, mas em vez de
somar tudo em um único contador, distribui os tamanhos de envio em 32 baldes
(potências de 2) usando um `BPF_MAP_TYPE_ARRAY` de 32 posições. É o mesmo
princípio usado por ferramentas como `bpftrace` para visualizar distribuições
de latência ou tamanho (seção 4.6 — mapas eBPF como estrutura central de
comunicação kernel ↔ espaço do usuário).

## Rodando (C + libbpf/CO-RE)

```bash
make
sudo ./build/hist
```

## Rodando (Python + BCC)

```bash
sudo python3 python/hist.py
```

Em ambos os casos, gere tráfego variado em outro terminal — por exemplo,
misture requisições pequenas ao `toy-server` (`curl`) com transferências
maiores — e observe as barras se redistribuírem a cada atualização (2s).
Testado de ponta a ponta nas duas versões.

## O que observar

- O cálculo do bucket roda **dentro do kernel**, a cada chamada de
  `tcp_sendmsg`. O espaço do usuário só lê os contadores já agregados —
  nenhum dado bruto por pacote cruza a fronteira kernel/usuário, o ponto
  central da seção 5.3.1 (eliminação do polling e das cópias desnecessárias).
- Na versão em C, o bucketing (`log2_aprox`) é escrito à mão, com um laço
  `#pragma unroll` — o verificador do eBPF precisa provar que todo programa
  termina (seção 3.3), e laços com limite fixo e conhecido em tempo de
  compilação são a forma mais simples de satisfazer essa exigência. Na
  versão em Python, o BCC oferece isso pronto: `BPF_HISTOGRAM` +
  `bpf_log2l()` fazem o bucketing, e `print_log2_hist()` cuida da formatação
  em ASCII no espaço do usuário — um bom exemplo de como o BCC prioriza
  produtividade onde libbpf/CO-RE prioriza portabilidade e menor overhead de
  carregamento.
