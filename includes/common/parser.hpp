#ifndef SJTU_PARSER_HPP
#define SJTU_PARSER_HPP

#include <string>

#include "common/util.hpp"

struct Command {
    int timestamp;
    std::string name;

    std::string args[26];
    bool has(char key) { return !args[key - 'a'].empty(); }
    const std::string& arg(char key) { return args[key - 'a']; }
};

inline Command parse_command(const std::string& line) {
    Command result;
    auto words = split(line);
    result.timestamp = to_int(words[0].substr(1, words[0].size() - 2));
    result.name = words[1];
    for (size_t i = 2; i < words.size(); i += 2) {
        result.args[words[i][1] - 'a'] = words[i + 1];
    }
    return result;
}

#endif
