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

#endif
