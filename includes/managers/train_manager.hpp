#ifndef TICKET_SYSTEM_TRAIN_MANAGER_HPP
#define TICKET_SYSTEM_TRAIN_MANAGER_HPP

#include <string>

#include "common/algorithm.hpp"
#include "common/date_time.hpp"
#include "common/types.hpp"
#include "common/util.hpp"
#include "containers/vector.hpp"
#include "storage/b_plus_tree.hpp"
#include "storage/memory_river.hpp"

class TrainManager {
   private:
    static constexpr int MAX_STATION = 100;

   public:
    struct Train {
        TrainID trainID;
        int station_num;
        Station stations[MAX_STATION];
        int seat_num;
        int prices[MAX_STATION];
        MinuteOfDate start_time;
        Duration travel_times[MAX_STATION - 1];
        Duration stopover_times[MAX_STATION - 2];
        Date sale_date[2];
        char type;
        bool released;

        Duration leave_offsets[MAX_STATION];
        Duration arrive_offsets[MAX_STATION];
        int price_prefix[MAX_STATION];
    };
    struct TrainSeat {
        int seats[MAX_STATION - 1];
    };
    struct TrainSeatKey {
        TrainID trainID;
        Date date;

        friend bool operator<(const TrainSeatKey& lhs,
                              const TrainSeatKey& rhs) {
            if (lhs.trainID < rhs.trainID) return true;
            if (rhs.trainID < lhs.trainID) return false;
            return lhs.date < rhs.date;
        }
    };

   private:
    MemoryRiver<Train, 0> trains_dat;
    BPlusTree<TrainID, int> train_index;
    MemoryRiver<TrainSeat, 0> train_seats_dat;
    BPlusTree<TrainSeatKey, int> train_seat_index;

    struct TrainRef {
        int train_idx;
        int station_idx;

        friend bool operator<(const TrainRef& lhs, const TrainRef& rhs) {
            if (lhs.train_idx != rhs.train_idx) {
                return lhs.train_idx < rhs.train_idx;
            }
            return lhs.station_idx < rhs.station_idx;
        }
    };
    BPlusTree<Station, TrainRef> train_station_index;

    bool get_train(const TrainID& trainID, Train& train, int& idx) {
        if (!train_index.find(trainID, idx)) {
            return false;
        }
        trains_dat.read(train, idx);
        return true;
    }
    bool get_train_seat(const TrainID& trainID, const Date& date,
                        TrainSeat& train_seat, int& idx) {
        if (!train_seat_index.find({trainID, date}, idx)) {
            return false;
        }
        // std::cout << "found, idx = " << idx << std::endl;
        train_seats_dat.read(train_seat, idx);
        return true;
    }

    template <class T>
    void parse_int_list(const std::string& str, T result[], int count) {
        auto values = split(str, '|');
        for (int i = 0; i < count; ++i) {
            result[i] = to_int(values[i]);
        }
    }

   public:
    TrainManager()
        : trains_dat("trains.dat"),
          train_index("train_index.idx"),
          train_seats_dat("train_seats.dat"),
          train_seat_index("train_seat.idx"),
          train_station_index("train_station.idx") {}

    void clean() {
        trains_dat.clean();
        train_index.clean();
        train_seats_dat.clean();
        train_seat_index.clean();
        train_station_index.clean();
    }

    bool add_train(const TrainID& trainID, int station_num, int seat_num,
                   const std::string& stations_str,
                   const std::string& prices_str,
                   const std::string& start_time_str,
                   const std::string& travel_times_str,
                   const std::string& stopover_times_str,
                   const std::string& sale_date_str, char type) {
        int idx = -1;
        if (train_index.find(trainID, idx)) {
            return false;
        }

        Train train;
        train.trainID = trainID;
        train.station_num = station_num;
        train.seat_num = seat_num;
        train.start_time = parse_minute_of_date(start_time_str);
        train.type = type;
        train.released = false;

        auto stations = split(stations_str, '|');
        for (int i = 0; i < station_num; ++i) {
            train.stations[i] = stations[i];
        }

        parse_int_list(prices_str, train.prices, station_num - 1);
        train.price_prefix[0] = 0;
        for (int i = 1; i < station_num; i++) {
            train.price_prefix[i] =
                train.price_prefix[i - 1] + train.prices[i - 1];
        }

        parse_int_list(travel_times_str, train.travel_times, station_num - 1);
        if (station_num > 2) {
            parse_int_list(stopover_times_str, train.stopover_times,
                           station_num - 2);
        }

        auto sale_dates = split(sale_date_str, '|');
        train.sale_date[0] = parse_date(sale_dates[0]);
        train.sale_date[1] = parse_date(sale_dates[1]);

        train.leave_offsets[0] = 0;
        train.arrive_offsets[0] = -1;
        for (int i = 1; i < station_num; ++i) {
            train.arrive_offsets[i] =
                train.leave_offsets[i - 1] + train.travel_times[i - 1];
            if (i < station_num - 1) {
                train.leave_offsets[i] =
                    train.arrive_offsets[i] + train.stopover_times[i - 1];
            }
        }
        train.leave_offsets[station_num - 1] = -1;

        const int new_idx = trains_dat.write(train);
        train_index.insert(trainID, new_idx);
        return true;
    }

    bool delete_train(const TrainID& trainID) {
        Train train;
        int idx = -1;
        if (!get_train(trainID, train, idx)) {
            return false;
        }
        if (train.released) {
            return false;
        }

        train_index.erase(trainID, idx);
        trains_dat.erase(idx);
        return true;
    }

    bool release_train(const TrainID& trainID) {
        Train train;
        int idx = -1;
        if (!get_train(trainID, train, idx)) {
            return false;
        }

        if (train.released) {
            return false;
        }
        for (Date d = train.sale_date[0]; d <= train.sale_date[1]; ++d) {
            TrainSeat train_seat;
            for (int i = 0; i < train.station_num - 1; ++i) {
                train_seat.seats[i] = train.seat_num;
            }
            int seat_idx = train_seats_dat.write(train_seat);
            // std::cout << "date = " << format_date(d) << ", idx = " <<
            // seat_idx
            //           << std::endl;
            train_seat_index.insert({trainID, d}, seat_idx);
        }

        for (int i = 0; i < train.station_num; i++) {
            train_station_index.insert(train.stations[i], {idx, i});
        }

        train.released = true;
        trains_dat.update(train, idx);
        return true;
    }

    bool query_train(const TrainID& trainID, const Date& date, Train& train,
                     TrainSeat& train_seat) {
        int train_idx = -1;
        if (!get_train(trainID, train, train_idx)) {
            return false;
        }
        if (!in_range(train.sale_date[0], train.sale_date[1], date)) {
            return false;
        }

        if (!train.released) {
            for (int i = 0; i < train.station_num - 1; i++) {
                train_seat.seats[i] = train.seat_num;
            }
        } else {
            int seat_idx = -1;
            get_train_seat(trainID, date, train_seat, seat_idx);
        }
        return true;
    }

    struct TicketResult {
        TrainID trainID;
        Station from, to;
        Time leaving_time;
        Time arriving_time;
        int price;
        int seat;
    };

    int query_seat(const TrainID& trainID, const Date& start_date, int from_idx,
                   int to_idx) {
        TrainSeat train_seat;
        int seat_idx = -1;
        if (!get_train_seat(trainID, start_date, train_seat, seat_idx)) {
            return 0;
        }
        return *min_element(train_seat.seats + from_idx,
                            train_seat.seats + to_idx);
    }

    sjtu::vector<TicketResult> query_ticket(const Station& from,
                                            const Station& to, const Date& date,
                                            const std::string& sorting_policy) {
        sjtu::vector<TicketResult> results;
        auto refs = train_station_index.find_all(from);
        for (const TrainRef& ref : refs) {
            Train train;
            trains_dat.read(train, ref.train_idx);
            int from_idx = ref.station_idx;
            int to_idx =
                static_cast<int>(find(train.stations + from_idx + 1,
                                      train.stations + train.station_num, to) -
                                 train.stations);
            if (to_idx == train.station_num) {
                continue;
            }

            Date start_date =
                date -
                (train.start_time + train.leave_offsets[from_idx]) / 1440;
            if (!in_range(train.sale_date[0], train.sale_date[1], start_date)) {
                continue;
            }

            Time train_start_time = make_time(start_date, train.start_time);
            results.push_back({.trainID = train.trainID,
                               .from = from,
                               .to = to,
                               .leaving_time = train_start_time +
                                               train.leave_offsets[from_idx],
                               .arriving_time = train_start_time +
                                                train.arrive_offsets[to_idx],
                               .price = train.price_prefix[to_idx] -
                                        train.price_prefix[from_idx],
                               .seat = query_seat(train.trainID, start_date,
                                                  from_idx, to_idx)});
        }

        if (sorting_policy == "cost") {
            sort(results.begin(), results.end(),
                 [](const TicketResult& lhs, const TicketResult& rhs) {
                     if (lhs.price != rhs.price) {
                         return lhs.price < rhs.price;
                     }
                     return lhs.trainID < rhs.trainID;
                 });
        } else {
            sort(results.begin(), results.end(),
                 [](const TicketResult& lhs, const TicketResult& rhs) {
                     Duration lhs_duration =
                         lhs.arriving_time - lhs.leaving_time;
                     Duration rhs_duration =
                         rhs.arriving_time - rhs.leaving_time;
                     if (lhs_duration != rhs_duration) {
                         return lhs_duration < rhs_duration;
                     }
                     return lhs.trainID < rhs.trainID;
                 });
        }
        return results;
    }

    bool query_transfer(const Station& from, const Station& to,
                        const Date& date, const std::string& sorting_policy,
                        TicketResult& first_leg, TicketResult& second_leg) {
        bool has_result = false;
        Duration best_duration = 0;
        int best_price = 0;
        Date best_first_start_date = 0;
        Date best_second_start_date = 0;
        int best_first_from_idx = 0;
        int best_first_to_idx = 0;
        int best_second_from_idx = 0;
        int best_second_to_idx = 0;

        auto first_refs = train_station_index.find_all(from);
        for (const TrainRef& first_ref : first_refs) {
            Train first_train;
            trains_dat.read(first_train, first_ref.train_idx);
            int first_from_idx = first_ref.station_idx;

            Date first_start_date =
                date - (first_train.start_time +
                        first_train.leave_offsets[first_from_idx]) /
                           1440;
            if (!in_range(first_train.sale_date[0], first_train.sale_date[1],
                          first_start_date)) {
                continue;
            }

            Time first_train_start_time =
                make_time(first_start_date, first_train.start_time);
            Time first_leaving_time = first_train_start_time +
                                      first_train.leave_offsets[first_from_idx];

            for (int first_to_idx = first_from_idx + 1;
                 first_to_idx < first_train.station_num; ++first_to_idx) {
                Station transfer_station = first_train.stations[first_to_idx];
                Time transfer_arriving_time =
                    first_train_start_time +
                    first_train.arrive_offsets[first_to_idx];
                int first_price = first_train.price_prefix[first_to_idx] -
                                  first_train.price_prefix[first_from_idx];
                Duration first_duration =
                    transfer_arriving_time - first_leaving_time;

                if (has_result) {
                    if (sorting_policy == "cost") {
                        if (first_price > best_price) {
                            break;
                        }
                    } else if (first_duration > best_duration) {
                        break;
                    }
                }

                TicketResult cur_first = {
                    .trainID = first_train.trainID,
                    .from = from,
                    .to = transfer_station,
                    .leaving_time = first_leaving_time,
                    .arriving_time = transfer_arriving_time,
                    .price = first_price,
                    .seat = 0};

                auto second_refs =
                    train_station_index.find_all(transfer_station);
                for (const TrainRef& second_ref : second_refs) {
                    Train second_train;
                    trains_dat.read(second_train, second_ref.train_idx);
                    if (second_train.trainID == first_train.trainID) {
                        continue;
                    }

                    int second_from_idx = second_ref.station_idx;
                    int second_to_idx = static_cast<int>(
                        find(second_train.stations + second_from_idx + 1,
                             second_train.stations + second_train.station_num,
                             to) -
                        second_train.stations);
                    if (second_to_idx == second_train.station_num) {
                        continue;
                    }

                    Duration second_depart_offset =
                        second_train.start_time +
                        second_train.leave_offsets[second_from_idx];
                    Date second_start_date =
                        (transfer_arriving_time - second_depart_offset + 1439) /
                        1440;
                    if (second_start_date < second_train.sale_date[0]) {
                        second_start_date = second_train.sale_date[0];
                    }
                    if (!in_range(second_train.sale_date[0],
                                  second_train.sale_date[1],
                                  second_start_date)) {
                        continue;
                    }

                    Time second_train_start_time =
                        make_time(second_start_date, second_train.start_time);
                    TicketResult cur_second = {
                        .trainID = second_train.trainID,
                        .from = transfer_station,
                        .to = to,
                        .leaving_time =
                            second_train_start_time +
                            second_train.leave_offsets[second_from_idx],
                        .arriving_time =
                            second_train_start_time +
                            second_train.arrive_offsets[second_to_idx],
                        .price = second_train.price_prefix[second_to_idx] -
                                 second_train.price_prefix[second_from_idx],
                        .seat = 0};

                    Duration cur_duration =
                        cur_second.arriving_time - cur_first.leaving_time;
                    int cur_price = cur_first.price + cur_second.price;
                    bool better = !has_result;
                    if (has_result) {
                        if (sorting_policy == "cost") {
                            if (cur_price != best_price) {
                                better = cur_price < best_price;
                            } else if (cur_duration != best_duration) {
                                better = cur_duration < best_duration;
                            } else if (cur_first.trainID != first_leg.trainID) {
                                better = cur_first.trainID < first_leg.trainID;
                            } else {
                                better =
                                    cur_second.trainID < second_leg.trainID;
                            }
                        } else {
                            if (cur_duration != best_duration) {
                                better = cur_duration < best_duration;
                            } else if (cur_price != best_price) {
                                better = cur_price < best_price;
                            } else if (cur_first.trainID != first_leg.trainID) {
                                better = cur_first.trainID < first_leg.trainID;
                            } else {
                                better =
                                    cur_second.trainID < second_leg.trainID;
                            }
                        }
                    }

                    if (better) {
                        has_result = true;
                        best_duration = cur_duration;
                        best_price = cur_price;
                        first_leg = cur_first;
                        second_leg = cur_second;
                        best_first_start_date = first_start_date;
                        best_second_start_date = second_start_date;
                        best_first_from_idx = first_from_idx;
                        best_first_to_idx = first_to_idx;
                        best_second_from_idx = second_from_idx;
                        best_second_to_idx = second_to_idx;
                    }
                }
            }
        }
        if (has_result) {
            first_leg.seat =
                query_seat(first_leg.trainID, best_first_start_date,
                           best_first_from_idx, best_first_to_idx);
            second_leg.seat =
                query_seat(second_leg.trainID, best_second_start_date,
                           best_second_from_idx, best_second_to_idx);
        }
        return has_result;
    }

    struct TicketPlan {
        Date start_date;
        Time leaving_time;
        Time arriving_time;
        int from_idx;
        int to_idx;
        int unit_price;
    };
    bool check_ticket(const TrainID& trainID, const Date& depart_date,
                      const Station& from, const Station& to, int num,
                      TicketPlan& out) {
        Train train;
        int train_idx = -1;
        if (!get_train(trainID, train, train_idx)) {
            return false;
        }
        int from_idx = static_cast<int>(
            find(train.stations, train.stations + train.station_num, from) -
            train.stations);
        if (from_idx == train.station_num) {
            return false;
        }
        if (num > train.seat_num) {
            return false;
        }
        int to_idx =
            static_cast<int>(find(train.stations + from_idx + 1,
                                  train.stations + train.station_num, to) -
                             train.stations);
        if (to_idx == train.station_num) {
            return false;
        }

        Date start_date =
            depart_date -
            (train.start_time + train.leave_offsets[from_idx]) / 1440;
        if (!in_range(train.sale_date[0], train.sale_date[1], start_date)) {
            return false;
        }
        Time train_start_time = make_time(start_date, train.start_time);
        out = {.start_date = start_date,
               .leaving_time = train_start_time + train.leave_offsets[from_idx],
               .arriving_time = train_start_time + train.arrive_offsets[to_idx],
               .from_idx = from_idx,
               .to_idx = to_idx,
               .unit_price =
                   train.price_prefix[to_idx] - train.price_prefix[from_idx]};
        return true;
    }

    // reserve num seats. return false if seat is not enough
    bool reserve_seats(const TrainID& trainID, const Date& start_date,
                       int from_idx, int to_idx, int num) {
        TrainSeat seat;
        int seat_idx = -1;
        if (!get_train_seat(trainID, start_date, seat, seat_idx)) {
            return false;
        }
        int available_seats =
            *min_element(seat.seats + from_idx, seat.seats + to_idx);
        if (available_seats < num) {
            return false;
        }

        for (int i = from_idx; i < to_idx; i++) {
            seat.seats[i] -= num;
        }
        train_seats_dat.update(seat, seat_idx);
        return true;
    }
    void restore_seats(const TrainID& trainID, const Date& start_date,
                       int from_idx, int to_idx, int num) {
        TrainSeat seat;
        int seat_idx = -1;
        if (!get_train_seat(trainID, start_date, seat, seat_idx)) {
            return;
        }
        for (int i = from_idx; i < to_idx; i++) {
            seat.seats[i] += num;
        }
        train_seats_dat.update(seat, seat_idx);
    }
};

#endif  // TICKET_SYSTEM_TRAIN_MANAGER_HPP
