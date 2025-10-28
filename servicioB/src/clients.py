import os, asyncio, httpx
from fastapi import HTTPException

SERVICE_A_BASE_URL = os.getenv("SERVICE_A_BASE_URL", "http://servicioa:8080")
MAX_RETRIES = int(os.getenv("MAX_RETRIES", "3"))

async def fetch_records(city: str, start_iso: str, end_iso: str) -> dict:
    last_err: Exception | None = None
    for attempt in range(MAX_RETRIES):
        try:
            async with httpx.AsyncClient(timeout=5) as client:
                r = await client.get(
                    f"{SERVICE_A_BASE_URL}/records",
                    params={"city": city, "from": start_iso, "to": end_iso, "limit": 1000},
                )
                if r.status_code == 200:
                    return r.json()
                # si A responde 4xx/5xx, no tiene sentido seguir reintentando salvo 5xx
                if 400 <= r.status_code < 500:
                    raise HTTPException(status_code=r.status_code, detail=r.text)
                last_err = HTTPException(status_code=r.status_code, detail=r.text)
        except Exception as e:
            last_err = e
        await asyncio.sleep(2 ** attempt)  # 1s, 2s, 4s

    raise HTTPException(status_code=503, detail=f"Servicio A no disponible: {last_err}")
