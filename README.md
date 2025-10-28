# Meteológica – Prueba Técnica Backend 2025

Sistema distribuido de ingestión y consulta de datos meteorológicos, desarrollado para la vacante Backend 2025.  
Incluye dos microservicios (C++ y Python), base de datos PostgreSQL y caché Redis, todo orquestado con Docker Compose.

---

## 🧩 Arquitectura General

```
Cliente / Frontend
        │
        ▼
 ┌─────────────────────────────┐
 │ Servicio B (Python / FastAPI)
 │ puerto 8090
 │ - API de consulta / agregación
 │ - Cambio de unidades (°C↔°F)
 │ - CORS configurado
 │ - Caché persistente (Redis)
 │ - Reintentos y tolerancia a fallos
 └────────────▲────────────────┘
              │
              ▼
 ┌─────────────────────────────┐
 │ Servicio A (C++ / libpqxx)
 │ puerto 8080
 │ - Ingesta CSV en PostgreSQL
 │ - Endpoints `/ingest/csv`, `/cities`, `/records`, `/health`
 └────────────▲────────────────┘
              │
              ▼
       PostgreSQL 18 (alpine)
              │
              ▼
            Redis 7 (alpine)
```

---

## 🚀 Puesta en marcha

### Requisitos
- Docker ≥ 24  
- Docker Compose v2  
- Archivo `meteo.csv` con datos meteorológicos de ejemplo

### Comandos principales

```bash
# Construir e iniciar todos los servicios
docker compose up -d --build

# Ver estado (todos deben aparecer "Up (healthy)")
docker compose ps

# Logs en vivo (Ctrl+C para salir)
docker compose logs -f
```

### Servicios y puertos

| Servicio | Tecnología | Puerto | Descripción |
|-----------|-------------|---------|--------------|
| `db` | PostgreSQL 18-alpine | 5432 | Base de datos `meteo` |
| `servicioa` | C++ / libpqxx | 8080 | API de ingestión |
| `serviciob` | Python / FastAPI | 8090 | API de consulta y agregación |
| `redis` | Redis 7-alpine | 6379 | Caché persistente |

---

## 📡 Endpoints principales

### Servicio A – C++
| Método | Endpoint | Descripción |
|--------|-----------|-------------|
| `GET` | `/health` | Estado de conexión con la BD |
| `POST` | `/ingest/csv` | Sube y almacena un CSV |
| `GET` | `/cities` | Lista de ciudades disponibles |
| `GET` | `/records` | Registros crudos por ciudad y rango |

### Servicio B – FastAPI
| Método | Endpoint | Descripción |
|--------|-----------|-------------|
| `GET` | `/health` | Estado del servicio y tipo de caché (`memory` o `redis`) |
| `GET` | `/weather/{city}` | Devuelve datos meteorológicos crudos o agregados |

#### Parámetros `/weather/{city}`

| Nombre | Tipo | Ejemplo | Descripción |
|--------|------|----------|-------------|
| `date` | string | `2025-10-15` | Fecha de inicio |
| `days` | int (1-10) | `5` | Días de consulta |
| `unit` | `C` / `F` | `C` | Unidad de temperatura |
| `agg` | `daily` / `rolling7` | `daily` | Tipo de agregación opcional |

---

## 🧠 Flujo de uso

### 1️⃣ Ingesta de datos (Servicio A)
```bash
curl -F "file=@meteo.csv" http://localhost:8080/ingest/csv
```

### 2️⃣ Consulta (Servicio B)
```bash
curl "http://localhost:8090/weather/Madrid?date=2025-10-15&days=5&unit=C"
```

Respuesta:
```json
{
  "city": "Madrid",
  "unit": "C",
  "from": "2025-10-15",
  "to": "2025-10-19",
  "days": [
    {"date": "2025-10-15", "temp_max_c": 15.75, "temp_min_c": 5.85, "precip_mm": 1.4, "cloud_pct": 80},
    ...
  ]
}
```

---

## ⚙️ Capa de Caché (Redis)

- Claves formadas como:  
  ```
  {city}:{start}:{days}:{unit}:{agg|none}
  ```
- TTL configurable vía `CACHE_TTL_SECONDS` (por defecto 600 s)

### Comprobación rápida
```bash
# Listar claves
docker compose exec redis redis-cli keys "*Madrid*"

# Ver contenido
docker compose exec redis redis-cli GET "Madrid:2025-10-15:5:C:none"

# TTL restante
docker compose exec redis redis-cli TTL "Madrid:2025-10-15:5:C:none"
```

---

## 🧩 Tolerancia a fallos

Servicio B reintenta las llamadas a A con backoff exponencial y sirve datos desde caché si A no está disponible.

### Prueba
```bash
# 1. Llenar caché
curl -s "http://localhost:8090/weather/Madrid?date=2025-10-15&days=5&unit=C" > /tmp/out1.json

# 2. Detener A
docker compose stop servicioa

# 3. Misma petición → servido desde caché
curl -s "http://localhost:8090/weather/Madrid?date=2025-10-15&days=5&unit=C" > /tmp/out2.json
diff /tmp/out1.json /tmp/out2.json || true

# 4. Petición nueva → 503
curl -i "http://localhost:8090/weather/Madrid?date=2025-10-16&days=5&unit=C"

# 5. Levantar A de nuevo
docker compose start servicioa
```

---

## 🌐 CORS y documentación

- CORS habilitado para `http://localhost:8090`, `http://localhost:3000`, `http://localhost:5173`
- Documentación interactiva:  
  👉 [http://localhost:8090/docs](http://localhost:8090/docs)  
  👉 [http://localhost:8090/redoc](http://localhost:8090/redoc)

---

## 🩺 Healthchecks automáticos

Cada contenedor tiene healthcheck configurado en `docker-compose.yml`.  
Compruébalos con:

```bash
docker compose ps
```

`Up (healthy)` → todo correcto.

---

## 🧱 Variables de entorno clave

| Variable | Servicio | Descripción | Ejemplo |
|-----------|-----------|-------------|----------|
| `DB_HOST`, `DB_PORT`, `DB_NAME`, `DB_USER`, `DB_PASSWORD` | A | Conexión PostgreSQL | `db`, `5432`, `meteo` |
| `SERVICE_A_BASE_URL` | B | URL interna de A | `http://servicioa:8080` |
| `CACHE_TTL_SECONDS` | B | Tiempo de vida en caché | `600` |
| `REDIS_URL` | B | Conexión Redis | `redis://redis:6379/0` |
| `ALLOW_ORIGINS` | B | Orígenes CORS permitidos | `http://localhost:3000,http://localhost:5173` |

---

## 🧪 Pruebas rápidas de rendimiento

```bash
time curl "http://localhost:8090/weather/Madrid?date=2025-10-15&days=5&unit=C"  # MISS
time curl "http://localhost:8090/weather/Madrid?date=2025-10-15&days=5&unit=C"  # HIT
```

---

## 📄 OpenAPI

FastAPI genera automáticamente la especificación:  
[`http://localhost:8090/openapi.json`](http://localhost:8090/openapi.json)

Para validadores que requieren 3.0.x, usa `openapi.yaml` con:
```yaml
openapi: 3.0.3
```

---

## 🧰 Tecnologías principales

- **C++ 17 (libpqxx, CMake, Debian Bookworm)**
- **Python 3.11 (FastAPI, Uvicorn, httpx, redis, docker-compose)**
- **PostgreSQL 18-alpine**
- **Redis 7-alpine**
- **Docker Compose v2**

---

## 📎 Licencia

Proyecto de prueba técnica – Uso interno educativo / de evaluación.  
© 2025 Alejandro Ruiz Salazar.
