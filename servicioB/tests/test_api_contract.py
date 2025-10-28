from fastapi.testclient import TestClient
import importlib

main = importlib.import_module("src.main")
app = main.app
client = TestClient(app)

def test_contract_weather_ok(monkeypatch):
    # Stub del fetch_records que usa main.py (importa de src.clients)
    async def fake_fetch(city, start_iso, end_iso):
        return {"items":[
            {"date":"2025-10-15","temp_max_c":16.5,"temp_min_c":8.1,"precip_mm":1.4,"cloud_pct":80},
            {"date":"2025-10-16","temp_max_c":17.0,"temp_min_c":7.9,"precip_mm":0.0,"cloud_pct":50},
        ]}
    # Intenta parchear el símbolo que main realmente usa
    if hasattr(main, "fetch_records"):
        monkeypatch.setattr(main, "fetch_records", fake_fetch)
    else:
        # si main hace import de módulo: from src import clients; clients.fetch_records(...)
        if hasattr(main, "clients"):
            monkeypatch.setattr(main.clients, "fetch_records", fake_fetch)

    r = client.get("/weather/Madrid?date=2025-10-15&days=2&unit=C&agg=daily")
    assert r.status_code == 200
    js = r.json()
    assert js["city"] == "Madrid"
    assert len(js["days"]) == 2
    # comprueba que los campos agregados existan
    first = js["days"][0]
    assert ("temp_avg_c" in first or "temp_mean_c" in first) and "precip_total_mm" in first
