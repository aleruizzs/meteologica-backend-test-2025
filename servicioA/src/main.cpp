#define CPPHTTPLIB_NO_EXCEPTIONS // (Opcional, pero recomendado en entornos C++)
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>
#include "httplib.h"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <pqxx/pqxx>

#include "db_config.h"
#include "utils.h"

using namespace std;
using ordered_json = nlohmann::ordered_json;

struct ParsedRow {
    std::string date_iso;  // YYYY-MM-DD
    std::string city;
    double temp_max_c;
    double temp_min_c;
    double precip_mm;
    int    cloud_pct;
};

int main(int argc, char** argv){
    DBconfig config = build_config();

    // Muestra por pantalla los datos de la BD, ocultando la contraseña
    //cout << config << endl;

    // Muestra por pantalla todos los datos de la BD, solo para depurar
    string conninfo = build_conninfo(config);
    //cout << conninfo << endl;
        
    if (argc > 1 && string(argv[1]) == "--server"){
        httplib::Server svr;
        svr.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
            nlohmann::json j;
            if (check_db(conninfo)) {
                j = { {"status", "DB OK"} };
                res.status = 200;
            } else {
                j = { {"error", "database unavailable"} };
                res.status = 503;
            }
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(j.dump(), "application/json");
        });
        svr.Post("/ingest/csv", [&](const httplib::Request& req, httplib::Response& res) {
            // 1) Extraer CSV (raw body o multipart/form-data)
            std::string csv_payload;
            if (req.is_multipart_form_data()) {
                const auto& form = req.form;
                bool found_data = false;
                const char* candidate_fields[] = { "file", "csv", "upload" };
                for (const char* field : candidate_fields) {
                    if (form.has_file(field)) {
                        csv_payload = form.get_file(field).content;
                        found_data = true;
                        break;
                    }
                }
                if (!found_data) {
                    for (const char* field : candidate_fields) {
                        if (form.has_field(field)) {
                            csv_payload = form.get_field(field);
                            found_data = true;
                            break;
                        }
                    }
                }
                if (!found_data && !form.files.empty()) {
                    csv_payload = form.files.begin()->second.content;
                    found_data = true;
                }
                if (!found_data && !form.fields.empty()) {
                    csv_payload = form.fields.begin()->second.content;
                    found_data = true;
                }
                if (!found_data) {
                    res.status = 400;
                    const char* err_json = "{\"error\":\"missing csv payload in multipart form\"}";
                    res.set_content(err_json, "application/json");
                    return;
                }
            } else {
                csv_payload = req.body;
            }
            if (csv_payload.empty()) {
                res.status = 400;
                const char* err_json = "{\"error\":\"no csv data provided; send raw body or multipart file\"}";
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content(err_json, "application/json");
                return;
            }

            // 2) Medir tiempo
            auto t0 = std::chrono::steady_clock::now();

            // 3) SHA-256 del cuerpo
            unsigned char hash[SHA256_DIGEST_LENGTH];
            SHA256(reinterpret_cast<const unsigned char*>(csv_payload.data()),
                csv_payload.size(), hash);

            std::ostringstream hex;
            hex << std::hex << std::setfill('0');
            for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
                hex << std::setw(2) << static_cast<int>(hash[i]);
            }
            std::string checksum = "sha256:" + hex.str();

            // Parseo y validacion de filas
            std::istringstream csv(csv_payload);
            std::string line;

            // 1) leer cabecera y comprobar columnas
            if (!std::getline(csv, line)) {
                res.status = 400;
                const char* err_json = "{\"error\":\"empty csv (no header)\"}";
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content(err_json, "application/json");
                return;
            }

            // Opcional: validar cabecera esperada (no obligatorio)
            // Esperado: Fecha;Ciudad;Temperatura Máxima (C);Temperatura Mínima (C);Precipitación (mm);Nubosidad (%)

            int rows_detected = 0;
            int rows_valid = 0;
            int rows_rejected = 0;
            std::vector<ParsedRow> valid_rows;
            valid_rows.reserve(256);

            while (std::getline(csv, line)) {
                if (line.empty()) continue;
                ++rows_detected;

                auto cols = utils::split_semicolon(line);
                if (cols.size() != 6) { rows_rejected++; continue; }

                for (auto& c : cols) c = utils::trim(c);

                ParsedRow r;
                // Fecha
                if (!utils::to_iso_date(cols[0], r.date_iso)) { rows_rejected++; continue; }
                // Ciudad
                r.city = cols[1];
                if (r.city.empty()) { rows_rejected++; continue; }
                // Temp max
                if (!utils::to_double_comma(cols[2], r.temp_max_c)) { rows_rejected++; continue; }
                // Temp min
                if (!utils::to_double_comma(cols[3], r.temp_min_c)) { rows_rejected++; continue; }
                // Precip
                if (!utils::to_double_comma(cols[4], r.precip_mm)) { rows_rejected++; continue; }
                if (r.precip_mm < 0.0) { rows_rejected++; continue; }
                // Nubosidad
                if (!utils::to_int(cols[5], r.cloud_pct)) { rows_rejected++; continue; }
                if (r.cloud_pct < 0 || r.cloud_pct > 100) { rows_rejected++; continue; }
                // Rango temperaturas
                if (r.temp_min_c > r.temp_max_c) { rows_rejected++; continue; }

                valid_rows.push_back(std::move(r));
                rows_valid++;
            }
            if (rows_detected == 0) {
                res.status = 400;
                const char* err_json = "{\"error\":\"empty csv (header only or no data rows)\"}";
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content(err_json, "application/json");
                return;
            }

            // Insercion en DB
            int rows_inserted = 0;
            int conflicts = 0;

            try {
                pqxx::connection c(conninfo);
                pqxx::work tx(c);

                // INSERT + ON CONFLICT DO NOTHING + RETURNING 1
                const char* sql =
                    "INSERT INTO weather_readings "
                    "(date, city, temp_max_c, temp_min_c, precip_mm, cloud_pct) "
                    "VALUES ($1,$2,$3,$4,$5,$6) "
                    "ON CONFLICT (city, date) DO NOTHING "
                    "RETURNING 1";

                for (const auto& r : valid_rows) {
                    auto r2 = tx.exec_params(sql,
                                            r.date_iso,   // 'YYYY-MM-DD'
                                            r.city,
                                            r.temp_max_c,
                                            r.temp_min_c,
                                            r.precip_mm,
                                            r.cloud_pct);
                    if (!r2.empty()) rows_inserted++;
                    else             conflicts++;   // conflicto UNIQUE (city,date) -> no inserta
                }

                tx.commit();
            } catch (const std::exception& e) {
                // DB caida o error de conexion/SQL
                ordered_json jerr{
                    {"error", "database unavailable"},
                    {"details", e.what()}
                };
                res.status = 503;
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content(jerr.dump(), "application/json");
                return;
            }

            // Rechazadas totales = invalidas (parseo) + conflictos por duplicado
            int rows_rejected_total = rows_rejected + conflicts;

            // === Tiempo total (incluye parseo + inserción) ===
            int elapsed_ms = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - t0
                ).count()
            );

            // === RESPUESTA FINAL (4 campos requeridos) ===
            ordered_json j{
                {"rows_inserted", rows_inserted},
                {"rows_rejected", rows_rejected_total},
                {"elapsed_ms",    elapsed_ms},
                {"file_checksum", checksum}
            };
            res.status = 200;
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(j.dump(), "application/json");
            return;

        });
        svr.Get("/cities", [&](const httplib::Request&, httplib::Response& res) {
            try {
                pqxx::connection c(conninfo);
                pqxx::work tx(c);
                pqxx::result r = tx.exec("SELECT DISTINCT city FROM weather_readings ORDER BY city ASC");
                
                vector<string> cities;
                for (const auto& row : r) {
                    cities.push_back(row["city"].c_str());
                }

                nlohmann::ordered_json j{{"cities", cities}};
                res.set_header("Access-Control-Allow-Origin", "*");
                res.status = 200;
                res.set_content(j.dump(), "application/json");
            } catch (const std::exception& e) {
                nlohmann::ordered_json jerr{
                    {"error", "database unavailable"},
                    {"details", e.what()}
                };
                res.status = 503;
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content(jerr.dump(), "application/json");
            }
        });
        svr.Get("/records", [&](const httplib::Request& req, httplib::Response& res) {
            using nlohmann::ordered_json;

            // -------- 1) Validación de parámetros --------
            auto city_it = req.params.find("city");
            auto from_it = req.params.find("from");
            auto to_it   = req.params.find("to");

            if (city_it == req.params.end() || from_it == req.params.end() || to_it == req.params.end()) {
                ordered_json jerr{
                    {"error", "missing required query parameters"},
                    {"required", {"city","from","to"}}
                };
                res.status = 400;
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content(jerr.dump(), "application/json");
                return;
            }

            std::string city = city_it->second;
            std::string from_s = from_it->second;
            std::string to_s   = to_it->second;

            // Normaliza/valida fechas (YYYY-MM-DD)
            std::string from_iso, to_iso;
            if (!utils::to_iso_date(from_s, from_iso) || !utils::to_iso_date(to_s, to_iso)) {
                ordered_json jerr{
                    {"error", "invalid date format"},
                    {"hint",  "use YYYY-MM-DD"}
                };
                res.status = 400;
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content(jerr.dump(), "application/json");
                return;
            }
            if (from_iso > to_iso) {
                ordered_json jerr{{"error", "`from` must be <= `to`"}};
                res.status = 400;
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content(jerr.dump(), "application/json");
                return;
            }

            // page & limit (opcionales)
            int page  = 1;
            int limit = 10;
            if (auto it = req.params.find("page"); it != req.params.end()) {
                if (!utils::to_int(it->second, page) || page < 1) page = 1;
            }
            if (auto it = req.params.find("limit"); it != req.params.end()) {
                if (!utils::to_int(it->second, limit) || limit < 1) limit = 10;
            }
            // Limitar el tamaño máximo por seguridad
            if (limit > 100) limit = 100;

            long long offset = static_cast<long long>((page - 1)) * static_cast<long long>(limit);

            // -------- 2) Consulta a BD (count + page) --------
            try {
                pqxx::connection c(conninfo);
                pqxx::work tx(c);

                // total de filas para esa ciudad/intervalo
                auto rcount = tx.exec_params(
                    "SELECT COUNT(*) AS cnt "
                    "FROM weather_readings "
                    "WHERE city = $1 AND date >= $2 AND date <= $3",
                    city, from_iso, to_iso
                );
                long long total = 0;
                if (!rcount.empty()) {
                    total = rcount[0]["cnt"].as<long long>(0);
                }

                // datos paginados (ordenados por fecha asc)
                auto rpage = tx.exec_params(
                    "SELECT date, temp_max_c, temp_min_c, precip_mm, cloud_pct "
                    "FROM weather_readings "
                    "WHERE city = $1 AND date >= $2 AND date <= $3 "
                    "ORDER BY date ASC "
                    "LIMIT $4 OFFSET $5",
                    city, from_iso, to_iso, limit, offset
                );

                // construir array de items
                std::vector<ordered_json> items;
                items.reserve(rpage.size());
                for (const auto& row : rpage) {
                    ordered_json item{
                        {"date",       row["date"].c_str()},
                        {"temp_max_c", row["temp_max_c"].as<double>()},
                        {"temp_min_c", row["temp_min_c"].as<double>()},
                        {"precip_mm",  row["precip_mm"].as<double>()},
                        {"cloud_pct",  row["cloud_pct"].as<int>()}
                    };
                    items.push_back(std::move(item));
                }

                // total_pages
                long long total_pages = (total == 0) ? 0 : ((total + limit - 1) / limit);

                ordered_json jout{
                    {"city",        city},
                    {"from",        from_iso},
                    {"to",          to_iso},
                    {"page",        page},
                    {"limit",       limit},
                    {"total",       total},
                    {"total_pages", total_pages},
                    {"items",       items}
                };

                res.status = 200;
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content(jout.dump(), "application/json");

            } catch (const std::exception& e) {
                ordered_json jerr{
                    {"error", "database unavailable"},
                    {"details", e.what()}
                };
                res.status = 503;
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content(jerr.dump(), "application/json");
            }
        });

        std::cout << "HTTP server on :8080\n";
        svr.listen("0.0.0.0", 8080);
        return 0;
    }

    return 0;
}
