import os
import json
from datetime import datetime, timedelta
from typing import Optional

from fastapi import FastAPI, HTTPException, Query, Request
from fastapi.responses import JSONResponse, Response
from fastapi.middleware.cors import CORSMiddleware

from .aggregations import aggregate_daily, aggregate_rolling7
from .clients import fetch_records
from . import cache  # aget/aset asíncronos


def c_to_f(c: float) -> float:
    return round((c * 9 / 5) + 32, 2)


# ------------------------------------------------------------
# Configuración principal
# ------------------------------------------------------------
app = FastAPI(title="Servicio B – Weather API", version="1.0.0")

# --- CORS ---
ALLOW_ORIGINS = os.getenv("ALLOW_ORIGINS", "*")
ALLOW_METHODS = os.getenv("ALLOW_METHODS", "*")
ALLOW_HEADERS = os.getenv("ALLOW_HEADERS", "*")
EXPOSE_HEADERS = os.getenv("EXPOSE_HEADERS", "")
ALLOW_CREDENTIALS = os.getenv("ALLOW_CREDENTIALS", "false").lower() == "true"
CORS_MAX_AGE = int(os.getenv("CORS_MAX_AGE", "600"))

origins = [o.strip() for o in ALLOW_ORIGINS.split(",") if o.strip()]
methods = [m.strip() for m in ALLOW_METHODS.split(",") if m.strip()]
headers = [h.strip() for h in ALLOW_HEADERS.split(",") if h.strip()]
expose = [h.strip() for h in EXPOSE_HEADERS.split(",") if h.strip()]

allow_origins_cfg = ["*"] if (origins == ["*"] and not ALLOW_CREDENTIALS) else origins or ["*"]
allow_methods_cfg = ["*"] if methods == ["*"] else (methods or ["GET", "OPTIONS"])
allow_headers_cfg = ["*"] if headers == ["*"] else (headers or ["*"])

app.add_middleware(
    CORSMiddleware,
    allow_origins=allow_origins_cfg,
    allow_credentials=ALLOW_CREDENTIALS,
    allow_methods=allow_methods_cfg,
    allow_headers=allow_headers_cfg,
    expose_headers=expose,
    max_age=CORS_MAX_AGE,
)

# Responder preflight en cualquier ruta y asegurar ACAO en todas las respuestas
@app.middleware("http")
async def cors_shortcircuit_options(request: Request, call_next):
    if request.method == "OPTIONS":
        return Response(
            status_code=204,
            headers={
                "Access-Control-Allow-Origin": "*" if not ALLOW_CREDENTIALS else (allow_origins_cfg[0] if allow_origins_cfg else ""),
                "Access-Control-Allow-Methods": ", ".join(allow_methods_cfg) if allow_methods_cfg != ["*"] else "GET, POST, OPTIONS",
                "Access-Control-Allow-Headers": ", ".join(allow_headers_cfg) if allow_headers_cfg != ["*"] else "Content-Type, Authorization",
                "Access-Control-Max-Age": str(CORS_MAX_AGE),
            },
        )
    resp = await call_next(request)
    resp.headers.setdefault(
        "Access-Control-Allow-Origin",
        "*" if not ALLOW_CREDENTIALS else (allow_origins_cfg[0] if allow_origins_cfg else ""),
    )
    return resp
# ------------------------------------------------------------


CACHE_TTL = int(os.getenv("CACHE_TTL_SECONDS", "600"))


@app.get("/health")
async def health():
    """
    OK si el proceso está vivo.
    Incluye 'cache'='redis' si REDIS_URL responde a PING; 'memory' en caso contrario.
    """
    status = {"status": "ok", "cache": "memory"}
    try:
        from .cache import _get_redis  # type: ignore
        r = await _get_redis()
        if r:
            pong = await r.ping()
            if pong:
                status["cache"] = "redis"  # type: ignore[index]
    except Exception:
        pass
    return status


@app.get("/weather/{city}")
async def get_weather(
    city: str,
    date: str = Query(..., description="Fecha de inicio (YYYY-MM-DD)"),
    days: int = Query(5, ge=1, le=10),
    unit: str = Query("C", pattern="^[CFcf]$"),
    agg: Optional[str] = Query(None, pattern="^(daily|rolling7)$"),
):
    # 1) Validar fecha
    try:
        start = datetime.strptime(date, "%Y-%m-%d").date()
    except ValueError:
        raise HTTPException(400, "Invalid date (expected YYYY-MM-DD)")
    end = start + timedelta(days=days - 1)

    # 2) Caché (clave incluye ciudad/fecha/rango/unidad/agg)
    cache_key = f"{city}:{start}:{days}:{unit.upper()}:{agg or 'none'}"

    cached_str = await cache.aget(cache_key)
    if cached_str:
        try:
            return JSONResponse(content=json.loads(cached_str), headers={"X-Cache": "HIT"})
        except Exception:
            # si hay algo corrupto, lo ignoramos y seguimos
            pass

    # 3) Llamar al Servicio A con backoff
    raw = await fetch_records(city, str(start), str(end))
    items = raw.get("items", [])
    if not items:
        raise HTTPException(404, "No data for given city/date range")

    # 4) Agregaciones
    if agg == "daily":
        payload = aggregate_daily(items)
    elif agg == "rolling7":
        payload = aggregate_rolling7(items)
    else:
        payload = sorted(items, key=lambda it: it["date"])

    # 5) Conversión de unidades (mantiene sufijos *_c por compatibilidad)
    out_unit = unit.upper()
    if out_unit == "F":
        for it in payload:
            if "temp_max_c" in it:
                it["temp_max_c"] = c_to_f(it["temp_max_c"])
            if "temp_min_c" in it:
                it["temp_min_c"] = c_to_f(it["temp_min_c"])
            if "temp_avg_c" in it:
                it["temp_avg_c"] = c_to_f(it["temp_avg_c"])
            if "temp_avg7_c" in it:
                it["temp_avg7_c"] = c_to_f(it["temp_avg7_c"])

    # 6) Respuesta final + guardado en caché
    result = {
        "city": city,
        "unit": out_unit,
        "from": str(start),
        "to": str(end),
        "days": payload,
    }
    await cache.aset(cache_key, json.dumps(result), CACHE_TTL)
    return JSONResponse(content=result, headers={"X-Cache": "MISS"})
