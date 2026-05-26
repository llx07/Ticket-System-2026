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
    BPlusTree<Username, int> order_username_index;

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
    void add_order(const Username& username, const Order& order) {
        // TODO
    }
    sjtu::vector<Order> query_orders(const Username& username) {
        // TODO
    }
    bool refund_nth_order(const Username& username, int nth,
                          Order& refunded_order) {
        // TODO
    }
    sjtu::vector<int> get_pending_orders(const TrainID& train_id, Date date) {
        // TODO
    }
    bool get_order_by_idx(int order_idx, Order& order) {
        // TODO
    }
    void mark_pending_success(int order_idx) {
        // TODO
    }
};
#endif  // TICKET_SYSTEM_ORDER_SYSTEM_HPP
