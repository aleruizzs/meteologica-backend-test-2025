# ---------- dev: entorno interactivo con bash ----------
FROM debian:bookworm-slim AS dev
WORKDIR /app

COPY packages.txt .
RUN apt-get update \
 && xargs -r apt-get install -y --no-install-recommends < packages.txt \
 && rm -rf /var/lib/apt/lists/*

# venv fuera de /app para que no lo tape el bind mount
RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:${PATH}" \
    PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copiamos el código de la app para que uvicorn pueda importarlo
COPY src/ ./src/
COPY openapi/ ./openapi/

RUN useradd -m appuser && chown -R appuser:appuser /app
USER appuser

EXPOSE 8090
CMD ["bash"]

# ---------- tests: corre pytest ----------
FROM python:3.11-slim AS tests
WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt pytest pytest-asyncio httpx fastapi uvicorn
ENV PYTHONPATH=/app
COPY src/ ./src/
COPY tests/ ./tests/
CMD ["pytest","-q"]
