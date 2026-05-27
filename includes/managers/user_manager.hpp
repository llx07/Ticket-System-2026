#ifndef TICKET_SYSTEM_USER_MANAGER_HPP
#define TICKET_SYSTEM_USER_MANAGER_HPP

#include "common/optional.hpp"
#include "common/types.hpp"
#include "containers/map.hpp"
#include "storage/b_plus_tree.hpp"
#include "storage/memory_river.hpp"

class UserManager {
   public:
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

    struct OnlineInfo {
        int idx;
        Privilege privilege;
    };
    sjtu::map<Username, OnlineInfo> online;

    bool get_user(const Username& username, User& user, int& idx) {
        if (!username_index.find(username, idx)) {
            return false;
        }
        users_dat.read(user, idx);
        return true;
    }

   public:
    UserManager() : users_dat("users.dat"), username_index("username.idx") {}

    void clean() {
        users_dat.clean();
        username_index.clean();
        online.clear();
    }

    bool is_online(const Username& username) {
        return online.find(username) != online.end();
    }

    bool add_user(const Username& cur_username, const Username& username,
                  const Password& password, const Name& name,
                  const MailAddr& mail_addr, Privilege privilege) {
        if (users_dat.size() == 0) {  // first user
            privilege = 10;
        } else {
            auto it_cur = online.find(cur_username);
            if (it_cur == online.end() ||
                it_cur->second.privilege <= privilege) {
                return false;
            }
            int idx = -1;
            if (username_index.find(username, idx)) {
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
        int idx = -1;
        if (!get_user(username, user, idx)) {
            return false;
        }
        if (is_online(username)) {
            return false;
        }
        if (user.password != passsword) {
            return false;
        }
        online.insert(sjtu::pair<const Username, OnlineInfo>{
            username, {idx, user.privilege}});
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
        auto cur_it = online.find(cur_username);
        if (cur_it == online.end()) {
            return false;
        }
        if (cur_username == username) {
            users_dat.read(result, cur_it->second.idx);
        } else {
            int result_idx = -1;
            if (!get_user(username, result, result_idx)) {
                return false;
            }
        }

        if (!(result.privilege < cur_it->second.privilege ||
              cur_username == username)) {
            return false;
        }

        return true;
    };

    bool modify_profile(const Username& cur_username, const Username& username,
                        const Optional<Password>& password,
                        const Optional<Name>& name,
                        const Optional<MailAddr>& mail_addr,
                        Optional<Privilege> privilege, User& result) {
        auto cur_it = online.find(cur_username);
        if (cur_it == online.end()) {
            return false;
        }
        int result_idx = -1;
        if (cur_username == username) {
            result_idx = cur_it->second.idx;
            users_dat.read(result, result_idx);
        } else {
            if (!get_user(username, result, result_idx)) {
                return false;
            }
        }
        if (!(result.privilege < cur_it->second.privilege ||
              cur_username == username)) {
            return false;
        }
        if (privilege && *privilege >= cur_it->second.privilege) {
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
        users_dat.update(result, result_idx);
        if (cur_username == username && privilege) {
            cur_it->second.privilege = *privilege;
        }
        return true;
    }
};

#endif  // TICKET_SYSTEM_USER_MANAGER_HPP
