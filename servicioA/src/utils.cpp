#include "utils.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>
#include <string>
#include <utility>

namespace {

bool is_leap(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

bool valid_date(int y, int m, int d) {
    if (y < 1900 || y > 2100 || m < 1 || m > 12 || d < 1) {
        return false;
    }
    static const int mdays[12] = {31, 28, 31, 30, 31, 30,
                                  31, 31, 30, 31, 30, 31};
    int last = mdays[m - 1] + ((m == 2 && is_leap(y)) ? 1 : 0);
    return d <= last;
}

} // namespace

namespace utils {

void ltrim(std::string &s) {
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(),
                         [](unsigned char ch) { return !std::isspace(ch); }));
}

void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            s.end());
}

std::string trim(std::string s) {
    ltrim(s);
    rtrim(s);
    return s;
}

std::vector<std::string> split_semicolon(const std::string &line) {
    std::vector<std::string> out;
    std::string cur;
    std::istringstream ss(line);
    while (std::getline(ss, cur, ';')) {
        out.push_back(cur);
    }
    return out;
}

bool to_iso_date(std::string in, std::string &iso) {
    in = trim(std::move(in));
    for (auto &ch : in) {
        if (ch == '/') {
            ch = '-';
        }
    }
    if (in.size() != 10 || in[4] != '-' || in[7] != '-') {
        return false;
    }
    try {
        int y = std::stoi(in.substr(0, 4));
        int m = std::stoi(in.substr(5, 2));
        int d = std::stoi(in.substr(8, 2));
        if (!valid_date(y, m, d)) {
            return false;
        }
        iso = in;
        return true;
    } catch (...) {
        return false;
    }
}

bool to_double_comma(std::string s, double &out) {
    s = trim(std::move(s));
    std::replace(s.begin(), s.end(), ',', '.');
    if (s.empty()) {
        return false;
    }
    try {
        size_t idx = 0;
        out = std::stod(s, &idx);
        return idx == s.size();
    } catch (...) {
        return false;
    }
}

bool to_int(std::string s, int &out) {
    s = trim(std::move(s));
    if (s.empty()) {
        return false;
    }
    try {
        size_t idx = 0;
        long v = std::stol(s, &idx);
        if (idx != s.size()) {
            return false;
        }
        if (v < std::numeric_limits<int>::min() ||
            v > std::numeric_limits<int>::max()) {
            return false;
        }
        out = static_cast<int>(v);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace utils

