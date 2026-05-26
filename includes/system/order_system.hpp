#ifndef TICKET_SYSTEM_ORDER_SYSTEM_HPP
#define TICKET_SYSTEM_ORDER_SYSTEM_HPP
#include "common/date_time.hpp"
#include "common/types.hpp"
#include "containers/vector.hpp"
#include "storage/b_plus_tree.hpp"
#include "storage/memory_river.hpp"

class OrderSystem {
   public:
    enum class OrderStatus { SUCCESS, PENDING, REFUNDED };
    struct Order {
        OrderStatus status;
        TrainID trainID;
        Station from, to;
        int from_idx, to_idx;
        Date start_date;
        Time leaving_time, arriving_time;
        int price;
        int num;
    };

   private:
    MemoryRiver<Order, 0> orders_dat;

    struct OrderIdx {
        int key;
        friend bool operator<(const OrderIdx& lhs, const OrderIdx& rhs) {
            return lhs.key > rhs.key;
        }
    };
    BPlusTree<Username, OrderIdx> order_username_index;

    struct PendingKey {
        TrainID trainID;
        Date date;
        friend bool operator<(const PendingKey& lhs, const PendingKey& rhs) {
            if (lhs.trainID != rhs.trainID) {
                return lhs.trainID < rhs.trainID;
            }
            return lhs.date < rhs.date;
        }
    };
    BPlusTree<PendingKey, int> order_pending_index;

   public:
    OrderSystem()
        : orders_dat("orders.dat"),
          order_username_index("order_username.idx"),
          order_pending_index("order_pending.idx") {}
    void clean() {
        orders_dat.clean();
        order_username_index.clean();
        order_pending_index.clean();
    }
    void add_order(const Username& username, const Order& order) {
        int order_idx = orders_dat.write(order);
        order_username_index.insert(username, OrderIdx{order_idx});
        if (order.status == OrderStatus::PENDING) {
            order_pending_index.insert({order.trainID, order.start_date},
                                      order_idx);
        }
    }
    sjtu::vector<Order> query_orders(const Username& username) {
        const sjtu::vector<OrderIdx>& indexes =
            order_username_index.find_all(username);
        sjtu::vector<Order> result;
        result.reserve(indexes.size());
        for (auto order_idx : indexes) {
            Order order;
            orders_dat.read(order, order_idx.key);
            result.push_back(order);
        }
        return result;
    }
    bool refund_nth_order(const Username& username, int nth,
                          Order& refunded_order) {
        OrderIdx order_idx_obj{-1};
        if (!order_username_index.find_nth(username, nth, order_idx_obj)) {
            return false;
        }
        int order_idx = order_idx_obj.key;
        orders_dat.read(refunded_order, order_idx);
        if (refunded_order.status == OrderSystem::OrderStatus::REFUNDED) {
            return false;
        }
        auto original_status = refunded_order.status;
        refunded_order.status = OrderSystem::OrderStatus::REFUNDED;
        orders_dat.update(refunded_order, order_idx);
        refunded_order.status = original_status;
        return true;
    }
    sjtu::vector<int> get_pending_orders(const TrainID& trainID, Date date) {
        return order_pending_index.find_all(PendingKey{trainID, date});
    }
    void get_order_by_idx(int order_idx, Order& order) {
        orders_dat.read(order, order_idx);
    }
    void mark_pending_success(int order_idx) {
        Order order;
        orders_dat.read(order, order_idx);
        order.status = OrderStatus::SUCCESS;
        orders_dat.update(order, order_idx);
        order_pending_index.erase({order.trainID, order.start_date}, order_idx);
    }
};
#endif  // TICKET_SYSTEM_ORDER_SYSTEM_HPP
