CREATE TABLE IF NOT EXISTS public.weather_readings (
  id BIGSERIAL PRIMARY KEY,
  date DATE NOT NULL,
  city TEXT NOT NULL,
  temp_max DOUBLE PRECISION NOT NULL,
  temp_min DOUBLE PRECISION NOT NULL,
  precip_mm  DOUBLE PRECISION NOT NULL,
  cloud_pct  INTEGER NOT NULL,
  UNIQUE (city, date)
);

CREATE INDEX IF NOT EXISTS idx_weather_city_date
  ON public.weather_readings(city, date);

