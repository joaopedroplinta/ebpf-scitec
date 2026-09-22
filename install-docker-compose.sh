#!/usr/bin/env bash
# Instala o plugin `docker compose` sem sudo (para máquinas de laboratório
# sem privilégio de root), baixando o binário oficial pro cli-plugins do
# usuário. Roda de qualquer lugar, não depende de estar dentro do repo.
set -euo pipefail

DEST="$HOME/.docker/cli-plugins/docker-compose"
mkdir -p "$(dirname "$DEST")"

echo "==> Baixando docker compose..."
curl -SL https://github.com/docker/compose/releases/latest/download/docker-compose-linux-x86_64 -o "$DEST"
chmod +x "$DEST"

echo "==> Verificando..."
docker compose version
