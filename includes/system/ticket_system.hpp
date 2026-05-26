#ifndef TICKET_SYSTEM_TICKET_SYSTEM_HPP
#define TICKET_SYSTEM_TICKET_SYSTEM_HPP

#include <iostream>
#include <string>

#include "common/date_time.hpp"
#include "common/optional.hpp"
#include "common/parser.hpp"
#include "common/types.hpp"
#include "common/util.hpp"
#include "system/train_system.hpp"
#include "system/user_system.hpp"

class TicketSystem {
   private:
    UserSystem user_system;
    TrainSystem train_system;

    std::string handle_add_user(const Command& cmd) {
        const bool ok = user_system.add_user(
            cmd.arg('c'), cmd.arg('u'), cmd.arg('p'), cmd.arg('n'),
            cmd.arg('m'), static_cast<char>(to_int(cmd.arg('g'))));
        return ok ? "0" : "-1";
    }

    std::string handle_login(const Command& cmd) {
        const bool ok = user_system.login(cmd.arg('u'), cmd.arg('p'));
        return ok ? "0" : "-1";
    }

    std::string handle_logout(const Command& cmd) {
        const bool ok = user_system.logout(cmd.arg('u'));
        return ok ? "0" : "-1";
    }

    std::string format_user_profile(const UserSystem::User& user) {
        return user.username.to_string() + " " + user.name.to_string() + " " +
               user.mail_addr.to_string() + " " +
               to_string(static_cast<int>(user.privilege));
    }

    std::string handle_query_profile(const Command& cmd) {
        UserSystem::User result;
        const bool ok =
            user_system.query_profile(cmd.arg('c'), cmd.arg('u'), result);
        return ok ? format_user_profile(result) : "-1";
    }

    std::string handle_modify_profile(const Command& cmd) {
        Optional<Password> password;
        Optional<Name> name;
        Optional<MailAddr> mail_addr;
        Optional<Privilege> privilege;

        if (cmd.has('p')) {
            password = cmd.arg('p');
        }
        if (cmd.has('n')) {
            name = cmd.arg('n');
        }
        if (cmd.has('m')) {
            mail_addr = cmd.arg('m');
        }
        if (cmd.has('g')) {
            privilege = static_cast<char>(to_int(cmd.arg('g')));
        }

        UserSystem::User result;
        const bool ok =
            user_system.modify_profile(cmd.arg('c'), cmd.arg('u'), password,
                                       name, mail_addr, privilege, result);
        return ok ? format_user_profile(result) : "-1";
    }

    std::string handle_add_train(const Command& cmd) {
        const bool ok = train_system.add_train(
            cmd.arg('i'), to_int(cmd.arg('n')), to_int(cmd.arg('m')),
            cmd.arg('s'), cmd.arg('p'), cmd.arg('x'), cmd.arg('t'),
            cmd.arg('o'), cmd.arg('d'), cmd.arg('y')[0]);
        return ok ? "0" : "-1";
    }

    std::string handle_delete_train(const Command& cmd) {
        const bool ok = train_system.delete_train(cmd.arg('i'));
        return ok ? "0" : "-1";
    }

    std::string handle_release_train(const Command& cmd) {
        const bool ok = train_system.release_train(cmd.arg('i'));
        return ok ? "0" : "-1";
    }

    std::string format_train(const TrainSystem::Train& train,
                             const TrainSystem::TrainSeat& train_seat,
                             const Date& date) {
        Time start_time = make_time(date, train.start_time);
        std::string output = train.trainID.to_string() + " " + train.type;
        for (int i = 0; i < train.station_num; i++) {
            output += "\n";
            output += train.stations[i].to_string();
            output += " ";
            output +=
                (i == 0 ? "xx-xx xx:xx"
                        : format_time(start_time + train.arrive_offsets[i]));
            output += " -> ";
            output += (i == train.station_num - 1
                           ? "xx-xx xx:xx"
                           : format_time(start_time + train.leave_offsets[i]));
            output += " ";
            output += to_string(train.price_prefix[i]);
            output += " ";
            output +=
                (i == train.station_num - 1 ? "x"
                                            : to_string(train_seat.seats[i]));
        }
        return output;
    }

    std::string handle_query_train(const Command& cmd) {
        TrainSystem::Train train;
        TrainSystem::TrainSeat train_seat;
        Date date = parse_date(cmd.arg('d'));
        const bool ok =
            train_system.query_train(cmd.arg('i'), date, train, train_seat);
        return ok ? format_train(train, train_seat, date) : "-1";
    }

    std::string format_ticket_result(const TrainSystem::TicketResult& result,
                                     const std::string& from,
                                     const std::string& to) {
        std::string output;
        output += result.train_id.to_string();
        output += " ";
        output += from;
        output += " ";
        output += format_time(result.leave_time);
        output += " -> ";
        output += to;
        output += " ";
        output += format_time(result.arriving_time);
        output += " ";
        output += to_string(result.price);
        output += " ";
        output += to_string(result.seat);
        return output;
    }
    std::string handle_query_ticket(const Command& cmd) {
        Date date = parse_date(cmd.arg('d'));
        const auto& results =
            train_system.query_ticket(cmd.arg('s'), cmd.arg('t'), date,
                                      cmd.has('p') ? cmd.arg('p') : "time");
        std::string output;
        output += to_string(static_cast<int>(results.size()));
        for (const auto& result : results) {
            output +=
                '\n' + format_ticket_result(result, cmd.arg('s'), cmd.arg('t'));
        }
        return output;
    }

   public:
    std::string execute(const Command& cmd) {
        if (cmd.name == "add_user") return handle_add_user(cmd);
        if (cmd.name == "login") return handle_login(cmd);
        if (cmd.name == "logout") return handle_logout(cmd);
        if (cmd.name == "query_profile") return handle_query_profile(cmd);
        if (cmd.name == "modify_profile") return handle_modify_profile(cmd);
        if (cmd.name == "add_train") return handle_add_train(cmd);
        if (cmd.name == "delete_train") return handle_delete_train(cmd);
        if (cmd.name == "release_train") return handle_release_train(cmd);
        if (cmd.name == "query_train") return handle_query_train(cmd);
        if (cmd.name == "query_ticket") return handle_query_ticket(cmd);
        if (cmd.name == "exit") return "bye";
        return "not_implemented";
    }
};

#endif  // TICKET_SYSTEM_TICKET_SYSTEM_HPP
