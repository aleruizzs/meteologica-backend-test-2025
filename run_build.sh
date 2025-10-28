#!/usr/bin/env bash
set -e
echo "Apagando contenedores previos..."
docker compose down -v
echo "Levantando stack..."
docker compose up -d --build
echo "✅ Todo listo. Servicios corriendo:"
docker compose ps
