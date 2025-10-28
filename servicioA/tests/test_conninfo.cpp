#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../src/third_party/doctest.h"
#include "../src/db_config.h"
#include <string>

TEST_CASE("build_conninfo genera cadena válida para pqxx") {
    DBconfig cfg;
    cfg.host="db"; cfg.port=5432; cfg.dbname="meteo"; cfg.user="meteo"; cfg.pwd="meteo";
    auto s = build_conninfo(cfg);
    CHECK(s.find("host=db") != std::string::npos);
    CHECK(s.find("dbname=meteo") != std::string::npos);
    CHECK(s.find("user=meteo") != std::string::npos);
    CHECK(s.find("password=meteo") != std::string::npos);
}
