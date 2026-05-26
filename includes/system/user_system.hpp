#ifndef TICKET_SYSTEM_USER_SYSTEM_HPP
#define TICKET_SYSTEM_USER_SYSTEM_HPP

#include "common/fixed_string.hpp"
#include "common/optional.hpp"
#include "containers/map.hpp"
#include "storage/b_plus_tree.hpp"
#include "storage/memory_river.hpp"

class UserSystem {
   public:
    using Username = FixedString<21>;
    using Password = FixedString<31>;
    using Name = FixedString<16>;
    using MailAddr = FixedString<31>;
    using Privilege = char;

    struct User {
        Username username;
        Password password;
        Name name;
        MailAddr mail_addr;
        Privilege privilege;
    };

   private:
    MemoryRiver<User, 0> users_dat;
    BPlusTree<Username, int> username_index;
    sjtu::map<Username, bool> online;

    bool get_user(const Username& username, User& user) {
        int idx = -1;
        if (!username_index.find(username, idx)) {
            return false;
        }
        users_dat.read(user, idx);
        return true;
    }
    bool write_user(const Username& username, const User& user) {
        int idx = -1;
        if (!username_index.find(username, idx)) {
            return false;
        }
        users_dat.update(user, idx);
        return true;
    }
    bool is_online(const Username& username) {
        return online.find(username) != online.end();
    }

   public:
    UserSystem() : users_dat("users.dat"), username_index("username.idx") {}

    bool add_user(const Username& cur_username, const Username& username,
                  const Password& password, const Name& name,
                  const MailAddr& mail_addr, Privilege privilege) {
        if (users_dat.size() == 0) {  // first user
            privilege = 10;
        } else {
            if (!is_online(cur_username)) {
                return false;
            }
            int idx = -1;
            if (username_index.find(username, idx)) {
                return false;
            }

            User cur_user;
            get_user(cur_username, cur_user);
            if (cur_user.privilege <= privilege) {
                return false;
            }
        }

        User new_user{.username = username,
                      .password = password,
                      .name = name,
                      .mail_addr = mail_addr,
                      .privilege = privilege};
        int new_idx = users_dat.write(new_user);
        username_index.insert(username, new_idx);

        return true;
    }

    bool login(const Username& username, const Password& passsword) {
        User user;
        if (!get_user(username, user)) {
            return false;
        }
        if (is_online(username)) {
            return false;
        }
        if (user.password != passsword) {
            return false;
        }
        online.insert(sjtu::pair<const Username, bool>{username, true});
        return true;
    }

    bool logout(const Username& username) {
        if (!is_online(username)) {
            return false;
        }
        online.erase(online.find(username));
        return true;
    }

    bool query_profile(const Username& cur_username, const Username& username,
                       User& result) {
        if (!is_online(cur_username)) {
            return false;
        }
        User cur_user;
        if (!get_user(cur_username, cur_user)) {
            return false;
        }
        if (!get_user(username, result)) {
            return false;
        }

        if (result.privilege > cur_user.privilege) {
            return false;
        }

        return true;
    };

    bool modify_profile(const Username& cur_username, const Username& username,
                        const Optional<Password>& password,
                        const Optional<Name>& name,
                        const Optional<MailAddr>& mail_addr,
                        Optional<Privilege> privilege, User& result) {
        if (!is_online(cur_username)) {
            return false;
        }
        User cur_user;
        if (!get_user(cur_username, cur_user)) {
            return false;
        }
        if (!get_user(username, result)) {
            return false;
        }
        if (result.privilege > cur_user.privilege ||
            (privilege && *privilege > cur_user.privilege)) {
            return false;
        }

        if (password) {
            result.password = *password;
        }
        if (name) {
            result.name = *name;
        }
        if (mail_addr) {
            result.mail_addr = *mail_addr;
        }
        if (privilege) {
            result.privilege = *privilege;
        }
        write_user(username, result);
        return true;
    }
};

#endif  // TICKET_SYSTEM_USER_SYSTEM_HPP
