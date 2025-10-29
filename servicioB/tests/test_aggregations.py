from src.aggregations import aggregate_daily  # y, si la tienes, aggregate_rolling7

def test_aggregate_daily_basico():
    rows = [
        {"date":"2025-10-15","temp_max":16.5,"temp_min":8.1,"precip_mm":1.4,"cloud_pct":80},
        {"date":"2025-10-15","temp_max":18.0,"temp_min":7.0,"precip_mm":0.0,"cloud_pct":60},
        {"date":"2025-10-16","temp_max":17.0,"temp_min":7.9,"precip_mm":0.2,"cloud_pct":50},
    ]
    out = aggregate_daily(rows)
    # Debe haber 2 fechas agregadas
    assert [d["date"] for d in out] == ["2025-10-15", "2025-10-16"]

    d1 = out[0]
    # medias/sumas sobre el 15:
    # medias de temp (por fila) -> [(16.5+8.1)/2, (18.0+7.0)/2] = [12.3, 12.5] => media = 12.4
    assert d1["temp_avg"] == 12.4 or d1.get("temp_mean_c") == 12.4
    assert d1["precip_total_mm"] == 1.4 + 0.0
    # media de nubosidad = (80 + 60) / 2 = 70
    assert d1["cloud_avg_pct"] == 70 or d1.get("cloud_pct_mean") == 70

    d2 = out[1]
    assert d2["date"] == "2025-10-16"
    assert d2["temp_avg"] == round((17.0+7.9)/2, 2) or d2.get("temp_mean_c") == round((17.0+7.9)/2, 2)
    assert d2["precip_total_mm"] == 0.2
    assert d2["cloud_avg_pct"] == 50 or d2.get("cloud_pct_mean") == 50

def test_aggregate_rolling7_si_existe():
    # Si tu módulo tiene rolling7, pruébalo; si no, borra este test
    try:
        from src.aggregations import aggregate_rolling7
    except ImportError:
        return  # el test pasa (no existe la función en tu código)
    rows = [
        {"date":"2025-10-01","temp_max":16.0,"temp_min":8.0,"precip_mm":0.0,"cloud_pct":50},
        {"date":"2025-10-02","temp_max":17.0,"temp_min":9.0,"precip_mm":0.1,"cloud_pct":60},
        {"date":"2025-10-03","temp_max":18.0,"temp_min":10.0,"precip_mm":0.2,"cloud_pct":70},
        {"date":"2025-10-04","temp_max":19.0,"temp_min":11.0,"precip_mm":0.3,"cloud_pct":80},
        {"date":"2025-10-05","temp_max":20.0,"temp_min":12.0,"precip_mm":0.4,"cloud_pct":90},
        {"date":"2025-10-06","temp_max":21.0,"temp_min":13.0,"precip_mm":0.5,"cloud_pct":60},
        {"date":"2025-10-07","temp_max":22.0,"temp_min":14.0,"precip_mm":0.6,"cloud_pct":40},
        {"date":"2025-10-08","temp_max":23.0,"temp_min":15.0,"precip_mm":0.7,"cloud_pct":20},
    ]
    out = aggregate_rolling7(rows)
    # Debe empezar a devolver desde el 7º elemento (índice 6) => fecha "2025-10-07"
    assert len(out) == len(rows) - 6
    assert out[0]["date"] == "2025-10-07"
    # comprueba que devuelve las claves esperadas
    first = out[0]
    assert "temp_avg7" in first and "cloud_avg7_pct" in first and "precip_sum7_mm" in first
