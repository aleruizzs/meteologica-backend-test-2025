# ---------- builder: compila con CMake ----------
FROM debian:bookworm-slim AS builder
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake pkg-config libpqxx-dev libpq-dev ca-certificates \
    nlohmann-json3-dev \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B /build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build /build -j
RUN cp /build/servicioa /servicioa

# ---------- tester: ejecuta ctest ----------
FROM debian:bookworm-slim AS tester
RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake \
    libpqxx-dev libpq5 ca-certificates && \
    rm -rf /var/lib/apt/lists/*
WORKDIR /build
COPY --from=builder /build /build
CMD ["ctest","--output-on-failure"]

# ---------- dev: tu stage interactiva con bash ----------
FROM debian:bookworm-slim AS dev
WORKDIR /app
COPY packages.txt .
RUN apt-get update \
 && xargs -r apt-get install -y --no-install-recommends < packages.txt \
 && rm -rf /var/lib/apt/lists/*
RUN useradd -m appuser
USER appuser
EXPOSE 8080
CMD ["bash"]

# --- runtime limpio ---
FROM debian:bookworm-slim AS runtime
RUN apt-get update && apt-get install -y --no-install-recommends \
    libpqxx-dev libpq5 ca-certificates curl && \
    rm -rf /var/lib/apt/lists/*
COPY --from=builder /servicioa /usr/local/bin/servicioa
EXPOSE 8080
ENTRYPOINT ["/usr/local/bin/servicioa","--server"]
