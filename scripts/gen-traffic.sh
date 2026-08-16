#!/usr/bin/env bash
# Gera tráfego HTTP contra o toy-server (docker/toy-server) para que os
# exemplos eBPF tenham algo para observar durante a demonstração.
set -euo pipefail

HOST="${1:-localhost}"
PORT="${2:-8080}"
N="${3:-50}"

for _ in $(seq 1 "$N"); do
    curl -s "http://${HOST}:${PORT}/" > /dev/null
    sleep 0.2
done

echo "Enviadas $N requisições para ${HOST}:${PORT}."
