import os
import asyncio
import random
import httpx
from fastapi import HTTPException

SERVICE_A_BASE_URL = os.getenv("SERVICE_A_BASE_URL", "http://servicioa:8080")
MAX_RETRIES = int(os.getenv("MAX_RETRIES", "3"))
PER_REQ_TIMEOUT = float(os.getenv("PER_REQ_TIMEOUT", "5"))

async def fetch_records(city: str, start_iso: str, end_iso: str) -> dict:
    """
    Política profesional:
    - 200 -> devolver JSON
    - 4xx -> NO reintentar, propagar el mismo 4xx
    - 5xx/errores red -> reintentos con backoff exponencial + jitter, si agota -> 503
    """
    last_err: Exception | None = None

    for attempt in range(MAX_RETRIES):
        try:
            async with httpx.AsyncClient(timeout=PER_REQ_TIMEOUT) as client:
                r = await client.get(
                    f"{SERVICE_A_BASE_URL}/records",
                    params={"city": city, "from": start_iso, "to": end_iso, "limit": 1000},
                )
        except httpx.RequestError as e:
            # error de red/timeout -> candidato a reintento
            last_err = e
        else:
            if r.status_code == 200:
                return r.json()

            if 400 <= r.status_code < 500:
                # 4xx -> NO reintentar, propagar
                # (si quisieras tratar 429 aparte con Retry-After, sería aquí)
                raise HTTPException(status_code=r.status_code, detail=r.text)

            # 5xx -> reintento más adelante (si quedan intentos)
            last_err = HTTPException(status_code=503, detail=f"{r.status_code}: {r.text}")

        # Backoff exponencial con un poco de jitter (hasta 20% del delay)
        if attempt < MAX_RETRIES - 1:
            base = 2 ** attempt  # 1, 2, 4...
            jitter = base * random.uniform(0.0, 0.2)
            await asyncio.sleep(base + jitter)

    # Se agotaron los intentos
    raise HTTPException(status_code=503, detail=f"Servicio A no disponible: {last_err}")
