#ifndef SJTU_UTIL_HPP
#define SJTU_UTIL_HPP

#include <string>

#include "containers/vector.hpp"

inline sjtu::vector<std::string> split(const std::string &str) {
    sjtu::vector<std::string> result;
    std::string token;

    for (char c : str) {
        if (c == ' ') {
            if (!token.empty()) {
                result.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }

    if (!token.empty()) {
        result.push_back(token);
    }

    return result;
}

inline int to_int(const std::string &str) {
    int result = 0;
    for (char c : str) {
        result = result * 10 + c - '0';
    }
    return result;
}

inline std::string to_string(int value) {
    if (value == 0) return "0";

    bool negative = false;
    if (value < 0) {
        negative = true;
        value = -value;
    }

    char buffer[12];
    int len = 0;
    while (value > 0) {
        buffer[len++] = static_cast<char>('0' + value % 10);
        value /= 10;
    }
    if (negative) buffer[len++] = '-';

    std::string result;
    for (int i = len - 1; i >= 0; --i) {
        result += buffer[i];
    }
    return result;
}

#endif
