#ifndef TICKET_SYSTEM_TICKET_SYSTEM_HPP
#define TICKET_SYSTEM_TICKET_SYSTEM_HPP

#include <string>

#include "common/optional.hpp"
#include "common/parser.hpp"
#include "common/util.hpp"
#include "system/user_system.hpp"

class TicketSystem {
   private:
    UserSystem user_system;

    std::string handle_add_user(const Command& cmd) {
        const bool ok = user_system.add_user(
            cmd.arg('c'), cmd.arg('u'), cmd.arg('p'), cmd.arg('n'),
            cmd.arg('m'), to_int(cmd.arg('g')));
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
        Optional<UserSystem::Password> password;
        Optional<UserSystem::Name> name;
        Optional<UserSystem::MailAddr> mail_addr;
        Optional<UserSystem::Privilege> privilege;

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
            privilege = to_int(cmd.arg('g'));
        }

        UserSystem::User result;
        const bool ok =
            user_system.modify_profile(cmd.arg('c'), cmd.arg('u'), password,
                                       name, mail_addr, privilege, result);
        return ok ? format_user_profile(result) : "-1";
    }

   public:
    std::string execute(const Command& cmd) {
        if (cmd.name == "add_user") return handle_add_user(cmd);
        if (cmd.name == "login") return handle_login(cmd);
        if (cmd.name == "logout") return handle_logout(cmd);
        if (cmd.name == "query_profile") return handle_query_profile(cmd);
        if (cmd.name == "modify_profile") return handle_modify_profile(cmd);
        if (cmd.name == "exit") return "bye";
        return "not_implemented";
    }
};

#endif  // TICKET_SYSTEM_TICKET_SYSTEM_HPP
