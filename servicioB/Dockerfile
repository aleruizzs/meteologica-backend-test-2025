FROM debian:bookworm-slim

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

# instala deps Python (si cambias requirements.txt, se re-evaluará esta capa)
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# usuario no-root como en A
RUN useradd -m appuser && chown -R appuser:appuser /app
USER appuser

EXPOSE 8090

# Igual que A: dejamos un shell para trabajar dentro sin cerrar el contenedor
CMD ["bash"]
