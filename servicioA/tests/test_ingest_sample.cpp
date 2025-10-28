#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../src/third_party/doctest.h"
#include "../src/utils.h"
#include <sstream>
#include <string>
#include <vector>

struct ParsedRow {
    std::string date_iso, city; double tmax, tmin, precip; int cloud;
};

static bool parse_row(const std::string& line, ParsedRow& r){
    auto cols = utils::split_semicolon(line);
    if (cols.size()!=6) return false;
    for (auto& c: cols) c = utils::trim(c);

    // Tu to_iso_date acepta ISO directamente
    if (!utils::to_iso_date(cols[0], r.date_iso)) return false;

    r.city = cols[1]; if(r.city.empty()) return false;

    // Tu parser parece esperar punto decimal (no coma)
    if (!utils::to_double_comma(cols[2], r.tmax)) return false;   // usa la función que tengas para parsear con punto
    if (!utils::to_double_comma(cols[3], r.tmin)) return false;
    if (!utils::to_double_comma(cols[4], r.precip)) return false;

    if (!utils::to_int(cols[5], r.cloud)) return false;
    if (r.cloud < 0 || r.cloud > 100) return false;
    if (r.tmin > r.tmax) return false;
    return true;
}

TEST_CASE("Parser CSV mínimo válido (ISO + punto decimal)") {
    std::string csv =
        "Fecha;Ciudad;Temperatura Max (C);Temperatura Min (C);Precipitacion (mm);Nubosidad (%)\n"
        "2025-10-15;Madrid;16.5;8.1;1.4;80\n"
        "2025-10-16;Madrid;17.0;7.9;0.0;50\n";
    std::istringstream in(csv);
    std::string line;
    CHECK(std::getline(in, line)); // header

    int ok=0, bad=0;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        ParsedRow r;
        if (parse_row(line, r)) ok++; else bad++;
    }
    CHECK(ok == 2);
    CHECK(bad == 0);
}
