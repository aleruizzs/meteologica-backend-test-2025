#pragma once

#include <iosfwd>
#include <string>

struct DBconfig {
    std::string host;
    std::string port;
    std::string dbname;
    std::string user;
    std::string pwd;
};

DBconfig build_config();
std::string build_conninfo(const DBconfig &c);
bool check_db(const std::string &conninfo);
std::ostream &operator<<(std::ostream &os, const DBconfig &c);

