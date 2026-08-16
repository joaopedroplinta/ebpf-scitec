# 00 — Aquecimento com BCC

O menor programa eBPF possível: anexa um `TRACEPOINT_PROBE` ao evento
`syscalls:sys_enter_execve` (ver seção 4.3 do esqueleto do curso — tracepoints
são pontos estáveis de rastreamento) e imprime o PID de cada processo que
chama `execve()` no sistema.

Serve para mostrar, sem nenhuma etapa de compilação manual, o ciclo completo
descrito na seção 3.6: escrever o programa, carregá-lo no kernel, ele reagir a
eventos em tempo real. O BCC (`bcc`/`python3-bpfcc`) compila o texto C
embutido para bytecode eBPF na hora, usando o clang internamente — é por isso
que não há Makefile aqui, ao contrário dos exemplos seguintes (que usam
libbpf/CO-RE e exigem compilação explícita).

## Rodando

Dentro do container `dev` (ver `docker/README.md`) ou em um host com
`python3-bpfcc` instalado:

```bash
sudo python3 hello_execve.py
```

Em outro terminal, rode qualquer comando (`ls`, `curl`, etc.) e veja a linha
correspondente aparecer no terminal do coletor. `Ctrl+C` para encerrar.

## O que observar

- Nenhuma compilação prévia foi necessária — isso é conveniente para
  prototipagem rápida, mas custa tempo de carregamento a cada execução
  (comparar com o exemplo `01-hello-tracepoint`, que compila o bytecode uma
  única vez com clang/libbpf).
- `bpf_trace_printk` escreve em `/sys/kernel/debug/tracing/trace_pipe`, um
  canal simples e útil para depuração, mas não recomendado para produção
  (ver mapas eBPF nos exemplos seguintes, que são o mecanismo real de
  comunicação kernel ↔ espaço do usuário, seção 4.6).
