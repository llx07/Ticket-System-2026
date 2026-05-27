#ifndef TICKET_SYSTEM_EXECUTOR_HPP
#define TICKET_SYSTEM_EXECUTOR_HPP

#include <iostream>
#include <string>

#include "common/date_time.hpp"
#include "common/optional.hpp"
#include "common/parser.hpp"
#include "common/types.hpp"
#include "common/util.hpp"
#include "managers/order_manager.hpp"
#include "managers/train_manager.hpp"
#include "managers/user_manager.hpp"

class Executor {
   private:
    UserManager user_manager;
    TrainManager train_manager;
    OrderManager order_manager;

    void print_ok(bool ok) { std::cout << (ok ? "0" : "-1"); }

    void handle_add_user(const Command& cmd) {
        print_ok(user_manager.add_user(
            cmd.arg('c'), cmd.arg('u'), cmd.arg('p'), cmd.arg('n'),
            cmd.arg('m'), static_cast<char>(to_int(cmd.arg('g')))));
    }

    void handle_login(const Command& cmd) {
        print_ok(user_manager.login(cmd.arg('u'), cmd.arg('p')));
    }

    void handle_logout(const Command& cmd) {
        print_ok(user_manager.logout(cmd.arg('u')));
    }

    void print_user_profile(const UserManager::User& user) {
        std::cout << user.username << ' ' << user.name << ' ' << user.mail_addr
                  << ' ' << to_string(static_cast<int>(user.privilege));
    }

    void handle_query_profile(const Command& cmd) {
        UserManager::User result;
        if (!user_manager.query_profile(cmd.arg('c'), cmd.arg('u'), result)) {
            std::cout << "-1";
            return;
        }
        print_user_profile(result);
    }

    void handle_modify_profile(const Command& cmd) {
        Optional<Password> password;
        Optional<Name> name;
        Optional<MailAddr> mail_addr;
        Optional<Privilege> privilege;

        if (cmd.has('p')) password = cmd.arg('p');
        if (cmd.has('n')) name = cmd.arg('n');
        if (cmd.has('m')) mail_addr = cmd.arg('m');
        if (cmd.has('g')) privilege = static_cast<char>(to_int(cmd.arg('g')));

        UserManager::User result;
        if (!user_manager.modify_profile(cmd.arg('c'), cmd.arg('u'), password,
                                         name, mail_addr, privilege, result)) {
            std::cout << "-1";
            return;
        }
        print_user_profile(result);
    }

    void handle_add_train(const Command& cmd) {
        print_ok(train_manager.add_train(
            cmd.arg('i'), to_int(cmd.arg('n')), to_int(cmd.arg('m')),
            cmd.arg('s'), cmd.arg('p'), cmd.arg('x'), cmd.arg('t'),
            cmd.arg('o'), cmd.arg('d'), cmd.arg('y')[0]));
    }

    void handle_delete_train(const Command& cmd) {
        print_ok(train_manager.delete_train(cmd.arg('i')));
    }

    void handle_release_train(const Command& cmd) {
        print_ok(train_manager.release_train(cmd.arg('i')));
    }

    void print_train(const TrainManager::Train& train,
                     const TrainManager::TrainSeat& train_seat,
                     const Date& date) {
        Time start_time = make_time(date, train.start_time);
        std::cout << train.trainID << ' ' << train.type;
        for (int i = 0; i < train.station_num; i++) {
            std::cout << '\n'
                      << train_manager.get_station_name(train.station_ids[i])
                      << ' ';
            if (i == 0) {
                std::cout << "xx-xx xx:xx";
            } else {
                std::cout << format_time(start_time + train.arrive_offsets[i]);
            }
            std::cout << " -> ";
            if (i == train.station_num - 1) {
                std::cout << "xx-xx xx:xx";
            } else {
                std::cout << format_time(start_time + train.leave_offsets[i]);
            }
            std::cout << ' ' << to_string(train.price_prefix[i]) << ' ';
            if (i == train.station_num - 1) {
                std::cout << 'x';
            } else {
                std::cout << to_string(train_seat.seats[i]);
            }
        }
    }

    void handle_query_train(const Command& cmd) {
        TrainManager::Train train;
        TrainManager::TrainSeat train_seat;
        Date date = parse_date(cmd.arg('d'));
        if (!train_manager.query_train(cmd.arg('i'), date, train, train_seat)) {
            std::cout << "-1";
            return;
        }
        print_train(train, train_seat, date);
    }

    void print_ticket_result(const TrainManager::TicketResult& result) {
        std::cout << result.trainID << ' ' << result.from << ' '
                  << format_time(result.leaving_time) << " -> " << result.to
                  << ' ' << format_time(result.arriving_time) << ' '
                  << to_string(result.price) << ' ' << to_string(result.seat);
    }

    void handle_query_ticket(const Command& cmd) {
        Date date = parse_date(cmd.arg('d'));
        const auto& results =
            train_manager.query_ticket(cmd.arg('s'), cmd.arg('t'), date,
                                       cmd.has('p') ? cmd.arg('p') : "time");
        std::cout << to_string(static_cast<int>(results.size()));
        for (const auto& result : results) {
            std::cout << '\n';
            print_ticket_result(result);
        }
    }

    void handle_query_transfer(const Command& cmd) {
        Date date = parse_date(cmd.arg('d'));
        TrainManager::TicketResult first_leg, second_leg;
        if (!train_manager.query_transfer(cmd.arg('s'), cmd.arg('t'), date,
                                          cmd.has('p') ? cmd.arg('p') : "time",
                                          first_leg, second_leg)) {
            std::cout << '0';
            return;
        }
        print_ticket_result(first_leg);
        std::cout << '\n';
        print_ticket_result(second_leg);
    }

    void handle_buy_ticket(const Command& cmd) {
        Username username = cmd.arg('u');
        if (!user_manager.is_online(username)) {
            std::cout << "-1";
            return;
        }
        TrainID trainID = cmd.arg('i');
        Station from = cmd.arg('f');
        Station to = cmd.arg('t');
        int num = to_int(cmd.arg('n'));
        bool willing_to_queue = cmd.has('q') && cmd.arg('q') == "true";

        TrainManager::TicketPlan plan;
        if (!train_manager.check_ticket(trainID, parse_date(cmd.arg('d')), from,
                                        to, num, plan)) {
            std::cout << "-1";
            return;
        }

        if (!train_manager.reserve_seats(trainID, plan.start_date,
                                         plan.from_idx, plan.to_idx, num)) {
            if (!willing_to_queue) {
                std::cout << "-1";
                return;
            }
            order_manager.add_order(
                username, {.status = OrderManager::OrderStatus::PENDING,
                           .trainID = trainID,
                           .from = from,
                           .to = to,
                           .from_idx = plan.from_idx,
                           .to_idx = plan.to_idx,
                           .start_date = plan.start_date,
                           .leaving_time = plan.leaving_time,
                           .arriving_time = plan.arriving_time,
                           .price = plan.unit_price,
                           .num = num});
            std::cout << "queue";
            return;
        }

        order_manager.add_order(username,
                                {.status = OrderManager::OrderStatus::SUCCESS,
                                 .trainID = trainID,
                                 .from = from,
                                 .to = to,
                                 .from_idx = plan.from_idx,
                                 .to_idx = plan.to_idx,
                                 .start_date = plan.start_date,
                                 .leaving_time = plan.leaving_time,
                                 .arriving_time = plan.arriving_time,
                                 .price = plan.unit_price,
                                 .num = num});
        std::cout << to_string(plan.unit_price * num);
    }

    void handle_query_order(const Command& cmd) {
        Username username = cmd.arg('u');
        if (!user_manager.is_online(username)) {
            std::cout << "-1";
            return;
        }
        const auto orders = order_manager.query_orders(username);
        std::cout << to_string(static_cast<int>(orders.size()));
        for (const auto& order : orders) {
            std::cout << "\n[";
            if (order.status == OrderManager::OrderStatus::PENDING)
                std::cout << "pending";
            else if (order.status == OrderManager::OrderStatus::SUCCESS)
                std::cout << "success";
            else if (order.status == OrderManager::OrderStatus::REFUNDED)
                std::cout << "refunded";
            std::cout << "] " << order.trainID << ' ' << order.from << ' '
                      << format_time(order.leaving_time) << " -> " << order.to
                      << ' ' << format_time(order.arriving_time) << ' '
                      << to_string(order.price) << ' ' << to_string(order.num);
        }
    }

    void handle_refund_ticket(const Command& cmd) {
        Username username = cmd.arg('u');
        if (!user_manager.is_online(username)) {
            std::cout << "-1";
            return;
        }
        int nth = cmd.has('n') ? to_int(cmd.arg('n')) : 1;
        OrderManager::Order order;
        if (!order_manager.refund_nth_order(username, nth, order)) {
            std::cout << "-1";
            return;
        }
        if (order.status == OrderManager::OrderStatus::SUCCESS) {
            train_manager.restore_seats(order.trainID, order.start_date,
                                        order.from_idx, order.to_idx,
                                        order.num);
            const auto& pending_queue = order_manager.get_pending_orders(
                order.trainID, order.start_date);
            for (int order_idx : pending_queue) {
                order_manager.get_order_by_idx(order_idx, order);
                if (train_manager.reserve_seats(order.trainID, order.start_date,
                                                order.from_idx, order.to_idx,
                                                order.num)) {
                    order_manager.mark_pending_success(order_idx);
                }
            }
        }
        std::cout << '0';
    }

    void handle_clean() {
        user_manager.clean();
        train_manager.clean();
        order_manager.clean();
        std::cout << '0';
    }

   public:
    void execute(const Command& cmd) {
        std::cout << '[' << cmd.timestamp << "] ";
        if (cmd.name == "add_user")
            handle_add_user(cmd);
        else if (cmd.name == "login")
            handle_login(cmd);
        else if (cmd.name == "logout")
            handle_logout(cmd);
        else if (cmd.name == "query_profile")
            handle_query_profile(cmd);
        else if (cmd.name == "modify_profile")
            handle_modify_profile(cmd);
        else if (cmd.name == "add_train")
            handle_add_train(cmd);
        else if (cmd.name == "delete_train")
            handle_delete_train(cmd);
        else if (cmd.name == "release_train")
            handle_release_train(cmd);
        else if (cmd.name == "query_train")
            handle_query_train(cmd);
        else if (cmd.name == "query_ticket")
            handle_query_ticket(cmd);
        else if (cmd.name == "query_transfer")
            handle_query_transfer(cmd);
        else if (cmd.name == "buy_ticket")
            handle_buy_ticket(cmd);
        else if (cmd.name == "query_order")
            handle_query_order(cmd);
        else if (cmd.name == "refund_ticket")
            handle_refund_ticket(cmd);
        else if (cmd.name == "clean")
            handle_clean();
        else if (cmd.name == "exit")
            std::cout << "bye";
        else
            std::cout << "not_implemented";
        std::cout << '\n';
    }
};

#endif  // TICKET_SYSTEM_EXECUTOR_HPP
