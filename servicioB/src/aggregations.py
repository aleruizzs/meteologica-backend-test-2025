from statistics import mean
from datetime import datetime

def _parse_date(s: str) -> datetime:
    return datetime.strptime(s, "%Y-%m-%d")

def aggregate_daily(items: list[dict]) -> list[dict]:
    # items vienen con fechas (YYYY-MM-DD). agrupamos por fecha exacta.
    groups: dict[str, list[dict]] = {}
    for it in items:
        d = it["date"]
        groups.setdefault(d, []).append(it)

    out: list[dict] = []
    for d, vals in sorted(groups.items(), key=lambda kv: kv[0]):
        out.append({
            "date": d,
            "temp_min": min(v["temp_min"] for v in vals),
            "temp_max": max(v["temp_max"] for v in vals),
            "temp_avg": round(mean((v["temp_min"] + v["temp_max"]) / 2 for v in vals), 2),
            "precip_total_mm": round(sum(v["precip_mm"] for v in vals), 2),
            "cloud_avg_pct": round(mean(v["cloud_pct"] for v in vals), 2),
        })
    return out

def aggregate_rolling7(items: list[dict]) -> list[dict]:
    # aseguramos orden por fecha asc
    items_sorted = sorted(items, key=lambda it: _parse_date(it["date"]))
    temps = []   # avg diaria
    clouds = []  # %
    precs = []   # mm

    out: list[dict] = []
    for i, it in enumerate(items_sorted):
        temps.append((it["temp_min"] + it["temp_max"]) / 2)
        clouds.append(it["cloud_pct"])
        precs.append(it["precip_mm"])
        if i >= 6:
            w0, w1 = i - 6, i + 1
            out.append({
                "date": it["date"],
                "temp_avg7": round(mean(temps[w0:w1]), 2),
                "cloud_avg7_pct": round(mean(clouds[w0:w1]), 2),
                "precip_sum7_mm": round(sum(precs[w0:w1]), 2),
            })
    return out
