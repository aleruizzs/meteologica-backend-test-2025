FROM debian:bookworm-slim

WORKDIR /app

COPY packages.txt .
RUN apt-get update \
 && xargs -r apt-get install -y --no-install-recommends < packages.txt \
 && rm -rf /var/lib/apt/lists/*


RUN useradd -m appuser
USER appuser

EXPOSE 8080

CMD ["bash"]
