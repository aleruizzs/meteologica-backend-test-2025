#!/usr/bin/env bash
set -euo pipefail

echo ">> Construyendo imágenes de test..."
docker compose -f docker-compose.tests.yml build

echo ">> Levantando DB..."
docker compose -f docker-compose.tests.yml up -d db

echo ">> Esperando a la DB..."
until docker compose -f docker-compose.tests.yml ps db | grep -q "healthy"; do
  sleep 2
done

echo ">> Tests servicioA..."
set +e
docker compose -f docker-compose.tests.yml run --rm servicioa-tests
A_STATUS=$?
set -e

echo ">> Tests servicioB..."
set +e
docker compose -f docker-compose.tests.yml run --rm serviciob-tests
B_STATUS=$?
set -e

echo ">> Apagando..."
docker compose -f docker-compose.tests.yml down -v || true

if [[ $A_STATUS -ne 0 || $B_STATUS -ne 0 ]]; then
  echo "❌ Tests fallidos: servicioA=$A_STATUS, servicioB=$B_STATUS"
  exit 1
fi
echo "✅ Todos los tests han pasado"
