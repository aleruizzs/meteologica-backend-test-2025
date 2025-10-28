#include "db_config.h"

#include <cstdlib>
#include <iostream>
#include <pqxx/pqxx>

namespace {

std::string getEnvOr(const char *key, const char *fallback) {
    const char *val = std::getenv(key);
    return val ? std::string(val) : std::string(fallback);
}

} // namespace

DBconfig build_config() {
    DBconfig cfg;
    cfg.host = getEnvOr("DB_HOST", "localhost");
    cfg.port = getEnvOr("DB_PORT", "5432");
    cfg.dbname = getEnvOr("POSTGRES_DB", "meteo");
    cfg.user = getEnvOr("POSTGRES_USER", "meteo");
    cfg.pwd = getEnvOr("POSTGRES_PASSWORD", "");
    return cfg;
}

std::string build_conninfo(const DBconfig &c) {
    return "host=" + c.host + " port=" + c.port + " dbname=" + c.dbname +
           " user=" + c.user + " password=" + c.pwd;
}

bool check_db(const std::string &conninfo) {
    try {
        pqxx::connection c(conninfo);
        pqxx::work tx(c);
        auto row = tx.exec1("SELECT 1");
        int v = row[0].as<int>();
        return v == 1;
    } catch (const std::exception &e) {
        std::cerr << "DB ERROR: " << e.what() << "\n";
        return false;
    }
}

std::ostream &operator<<(std::ostream &os, const DBconfig &c) {
    os << "host=" << c.host << " port=" << c.port << " dbname=" << c.dbname
       << " user=" << c.user << " password=" << "***";
    return os;
}

