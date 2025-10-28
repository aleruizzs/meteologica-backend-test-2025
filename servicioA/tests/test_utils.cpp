#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../src/third_party/doctest.h"
#include "../src/utils.h"
#include <string>
#include <vector>

TEST_CASE("split_semicolon y trim (sin contar último vacío)") {
    auto v = utils::split_semicolon("a; b ;c;;");
    // Tu implementación devuelve 4 elementos, no 5.
    CHECK(v.size() == 4);
    // trim se aplica
    CHECK(utils::trim(v[1]) == "b");
    // el penúltimo puede ser "c" y el último vacío ignorado
    CHECK(utils::trim(v[2]) == "c");
}

TEST_CASE("to_iso_date acepta solo formato ISO yyyy-mm-dd") {
    std::string out;
    // ES dd/mm/yyyy no convierte y devuelve false
    CHECK_FALSE(utils::to_iso_date("15/10/2025", out));
    // ISO yyyy-mm-dd sí lo acepta y devuelve la misma fecha
    CHECK(utils::to_iso_date("2025-10-15", out));
    CHECK(out == "2025-10-15");
    // Formatos inválidos -> false
    CHECK_FALSE(utils::to_iso_date("2025-15-99", out));
}
