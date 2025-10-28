#pragma once

#include <string>
#include <vector>

namespace utils {

void ltrim(std::string &s);
void rtrim(std::string &s);
std::string trim(std::string s);

std::vector<std::string> split_semicolon(const std::string &line);

bool to_iso_date(std::string in, std::string &iso);
bool to_double_comma(std::string s, double &out);
bool to_int(std::string s, int &out);

} // namespace utils

