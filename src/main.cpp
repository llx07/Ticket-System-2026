#include <iostream>
#include <string>

#include "common/parser.hpp"
#include "common/executor.hpp"

int main() {
    std::ios::sync_with_stdio(0);
    std::cin.tie(0);

    Executor executor;
    std::string line;

    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        Command cmd = parse_command(line);
        executor.execute(cmd);

        if (cmd.name == "exit") break;
    }

    return 0;
}
