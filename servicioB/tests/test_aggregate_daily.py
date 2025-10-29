from src.aggregations import aggregate_daily

def test_daily_aggregation_simple():
    rows = [
        {"date":"2025-10-15","temp_max":16.5,"temp_min":8.1,"precip_mm":1.4,"cloud_pct":80},
        {"date":"2025-10-16","temp_max":17.0,"temp_min":7.9,"precip_mm":0.0,"cloud_pct":50},
    ]
    out = aggregate_daily(rows)

    assert out[0]["date"] == "2025-10-15"
    assert round(out[0]["temp_avg"], 2) == round((16.5 + 8.1) / 2, 2)
    assert out[0]["precip_total_mm"] == 1.4
    assert out[0]["cloud_avg_pct"] == 80

    assert out[1]["date"] == "2025-10-16"
    assert round(out[1]["temp_avg"], 2) == round((17.0 + 7.9) / 2, 2)
    assert out[1]["precip_total_mm"] == 0.0
    assert out[1]["cloud_avg_pct"] == 50
