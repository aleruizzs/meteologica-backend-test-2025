import os
import time
from typing import Any, Optional

REDIS_URL = os.getenv("REDIS_URL", "").strip()

# ---- Fallback en memoria (simple, con TTL) ----
_mem_store: dict[str, tuple[float, Any]] = {}

def _mem_get(key: str) -> Optional[Any]:
    item = _mem_store.get(key)
    if not item:
        return None
    exp, data = item
    if exp < time.time():
        _mem_store.pop(key, None)
        return None
    return data

def _mem_set(key: str, data: Any, ttl_seconds: int) -> None:
    _mem_store[key] = (time.time() + ttl_seconds, data)


# ---- Redis asíncrono (si REDIS_URL está configurado) ----
_redis = None

async def _get_redis():
    global _redis
    if _redis is not None:
        return _redis
    if not REDIS_URL:
        return None
    try:
        from redis import asyncio as aioredis
        _redis = aioredis.from_url(REDIS_URL, encoding="utf-8", decode_responses=True)
        # probamos conexión
        await _redis.ping()
        return _redis
    except Exception:
        _redis = None
        return None


# ---- API pública asíncrona ----
async def aget(key: str) -> Optional[Any]:
    """
    Obtiene desde Redis si está disponible; si no, desde memoria.
    """
    r = await _get_redis()
    if r:
        val = await r.get(key)
        if val is None:
            return None
        # guardamos JSON como string; main.py lo maneja ya como dict.
        # aquí devolvemos la string y el caller decidirá si es JSON/cached object.
        # Para evitar parsing doble, preferimos que main.py guarde/recupere JSON (str).
        return val
    # memoria
    return _mem_get(key)


async def aset(key: str, data: Any, ttl_seconds: int) -> None:
    """
    Guarda en Redis (con EX) si está disponible; si no, en memoria.
    """
    r = await _get_redis()
    if r:
        # asumimos que 'data' es string JSON (ver main.py). Si fuese dict, conviértelo antes de guardar.
        await r.set(key, data, ex=ttl_seconds)
        return
    _mem_set(key, data, ttl_seconds)
