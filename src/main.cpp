#include <iostream>
#include <string>

#include "common/parser.hpp"
#include "system/ticket_system.hpp"

int main() {
    std::ios::sync_with_stdio(0);
    std::cin.tie(0);

    TicketSystem system;
    std::string line;

    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        Command cmd = parse_command(line);
        std::cout << '[' << cmd.timestamp << "] " << system.execute(cmd)
                  << '\n';

        if (cmd.name == "exit") break;
    }

    return 0;
}
