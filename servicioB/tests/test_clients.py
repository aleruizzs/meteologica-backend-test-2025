import pytest
import httpx
from fastapi import HTTPException
from src import clients

class FakeResp:
    def __init__(self, status_code=200, json_data=None, text=""):
        self.status_code = status_code
        self._json = json_data if json_data is not None else {}
        self.text = text
    def json(self):
        return self._json

@pytest.mark.asyncio
async def test_fetch_records_reintentos_y_exito(monkeypatch):
    calls = {"n": 0}
    async def fake_get(url, params=None):
        calls["n"] += 1
        if calls["n"] < 3:
            raise httpx.RequestError("fail")
        return FakeResp(200, {"items":[]})

    class DummyClient:
        def __init__(self, *a, **k): pass
        async def __aenter__(self): return self
        async def __aexit__(self, *a): pass
        async def get(self, url, params=None): return await fake_get(url, params)

    monkeypatch.setattr(clients.httpx, "AsyncClient", DummyClient)

    js = await clients.fetch_records("Madrid", "2025-10-15", "2025-10-19")
    assert js["items"] == []
    assert calls["n"] == 3  # 2 fallos + 1 éxito

@pytest.mark.asyncio
async def test_fetch_records_4xx_no_reintenta(monkeypatch):
    calls = {"n": 0}
    async def fake_get(url, params=None):
        calls["n"] += 1
        return FakeResp(404, text="not found")

    class DummyClient:
        def __init__(self, *a, **k): pass
        async def __aenter__(self): return self
        async def __aexit__(self, *a): pass
        async def get(self, url, params=None): return await fake_get(url, params)

    monkeypatch.setattr(clients.httpx, "AsyncClient", DummyClient)

    with pytest.raises(HTTPException) as ei:
        await clients.fetch_records("Madrid", "2025-10-15", "2025-10-19")
    assert ei.value.status_code == 404   # Propagamos el 4xx tal cual
    assert calls["n"] == 1               # Y NO reintentamos

@pytest.mark.asyncio
async def test_fetch_records_5xx_agota_reintentos(monkeypatch):
    calls = {"n": 0}
    async def fake_get(url, params=None):
        calls["n"] += 1
        return FakeResp(500, text="server error")

    class DummyClient:
        def __init__(self, *a, **k): pass
        async def __aenter__(self): return self
        async def __aexit__(self, *a): pass
        async def get(self, url, params=None): return await fake_get(url, params)

    monkeypatch.setattr(clients.httpx, "AsyncClient", DummyClient)

    with pytest.raises(HTTPException) as ei:
        await clients.fetch_records("Madrid", "2025-10-15", "2025-10-19")
    assert ei.value.status_code == 503   # agotó reintentos en 5xx
    assert calls["n"] >= clients.MAX_RETRIES
