#ifndef TICKET_SYSTEM_TRAIN_MANAGER_HPP
#define TICKET_SYSTEM_TRAIN_MANAGER_HPP

#include <string>

#include "common/algorithm.hpp"
#include "common/date_time.hpp"
#include "common/types.hpp"
#include "common/util.hpp"
#include "containers/utility.hpp"
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
        int station_ids[MAX_STATION];
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
   private:
    MemoryRiver<Train, 0> trains_dat;
    BPlusTree<TrainID, int> train_index;
    MemoryRiver<TrainSeat, 0> train_seats_dat;
    BPlusTree<sjtu::pair<TrainID, Date>, int> train_seat_index;
    MemoryRiver<Station, 0> stations_dat;
    BPlusTree<Station, int> station_id_index;

    struct TrainRef {
        Date sale_date[2];
        TrainID trainID;
        Duration leave_offset;
        Duration arrive_offset;
        MinuteOfDate train_start_time;
        int price;
        int from_idx;
        int to_idx;

        friend bool operator<(const TrainRef& lhs, const TrainRef& rhs) {
            int lhs_arrive_time =
                lhs.train_start_time + lhs.arrive_offset -
                (lhs.train_start_time + lhs.leave_offset) / 1440 * 1440;
            int rhs_arrive_time =
                rhs.train_start_time + rhs.arrive_offset -
                (rhs.train_start_time + rhs.leave_offset) / 1440 * 1440;
            if (lhs_arrive_time != rhs_arrive_time) {
                return lhs_arrive_time < rhs_arrive_time;
            }
            if (lhs.trainID != rhs.trainID) {
                return lhs.trainID < rhs.trainID;
            }
            if (lhs.from_idx != rhs.from_idx) {
                return lhs.from_idx < rhs.from_idx;
            }
            return lhs.to_idx < rhs.to_idx;
        }
    };
    BPlusTree<sjtu::pair<int, int>, TrainRef> train_station_index;

    BPlusTree<int, int> reachable_to_id;
    BPlusTree<int, int> reachable_from_id;

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

    int get_or_create_station_id(const Station& station) {
        int station_id = -1;
        if (station_id_index.find(station, station_id)) {
            return station_id;
        }
        station_id = stations_dat.write(station);
        station_id_index.insert(station, station_id);
        return station_id;
    }

    bool get_station_id(const Station& station, int& station_id) {
        return station_id_index.find(station, station_id);
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
          stations_dat("stations.dat"),
          station_id_index("station_id.idx"),
          train_station_index("train_station.idx"),
          reachable_to_id("reachable_to.idx"),
          reachable_from_id("reachable_from.idx") {}

    void clean() {
        trains_dat.clean();
        train_index.clean();
        train_seats_dat.clean();
        train_seat_index.clean();
        stations_dat.clean();
        station_id_index.clean();
        train_station_index.clean();
        reachable_to_id.clean();
        reachable_from_id.clean();
    }

    Station get_station_name(int station_id) {
        Station station;
        stations_dat.read(station, station_id);
        return station;
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
            train.station_ids[i] = get_or_create_station_id(stations[i]);
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
            for (int j = i + 1; j < train.station_num; j++) {
                train_station_index.insert(
                    {train.station_ids[i], train.station_ids[j]},
                    {.sale_date = {train.sale_date[0], train.sale_date[1]},
                     .trainID = train.trainID,
                     .leave_offset = train.leave_offsets[i],
                     .arrive_offset = train.arrive_offsets[j],
                     .train_start_time = train.start_time,
                     .price = train.price_prefix[j] - train.price_prefix[i],
                     .from_idx = i,
                     .to_idx = j});
                reachable_to_id.insert(train.station_ids[i],
                                       train.station_ids[j]);
                reachable_from_id.insert(train.station_ids[j],
                                         train.station_ids[i]);
            }
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
        int from_id = -1, to_id = -1;
        if (!get_station_id(from, from_id) || !get_station_id(to, to_id)) {
            return results;
        }

        auto refs = train_station_index.find_all({from_id, to_id});
        for (const TrainRef& ref : refs) {
            int from_idx = ref.from_idx;
            int to_idx = ref.to_idx;

            Date start_date =
                date - (ref.train_start_time + ref.leave_offset) / 1440;
            if (!in_range(ref.sale_date[0], ref.sale_date[1], start_date)) {
                continue;
            }

            Time train_start_time = make_time(start_date, ref.train_start_time);
            results.push_back(
                {.trainID = ref.trainID,
                 .from = from,
                 .to = to,
                 .leaving_time = train_start_time + ref.leave_offset,
                 .arriving_time = train_start_time + ref.arrive_offset,
                 .price = ref.price,
                 .seat =
                     query_seat(ref.trainID, start_date, from_idx, to_idx)});
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

        int from_id = -1, to_id = -1;
        if (!get_station_id(from, from_id) || !get_station_id(to, to_id)) {
            return false;
        }

        auto reachable_to = reachable_to_id.find_all(from_id);
        auto reachable_from = reachable_from_id.find_all(to_id);
        size_t i = 0, j = 0;
        while (i < reachable_to.size() && j < reachable_from.size()) {
            int x = reachable_to[i];
            int y = reachable_from[j];
            if (x < y) {
                ++i;
                continue;
            }
            if (y < x) {
                ++j;
                continue;
            }

            int transfer_station_id = x;
            ++i, ++j;
            auto first_refs =
                train_station_index.find_all({from_id, transfer_station_id});
            if (first_refs.empty()) {
                continue;
            }
            auto second_refs =
                train_station_index.find_all({transfer_station_id, to_id});
            if (second_refs.empty()) {
                continue;
            }

            Station transfer_station = get_station_name(transfer_station_id);
            int best_first_price_so_far = 0x7fffffff;
            for (const TrainRef& first_ref : first_refs) {
                int first_from_idx = first_ref.from_idx;
                int first_to_idx = first_ref.to_idx;

                Date first_start_date = date - (first_ref.train_start_time +
                                                first_ref.leave_offset) /
                                                   1440;
                if (!in_range(first_ref.sale_date[0], first_ref.sale_date[1],
                              first_start_date)) {
                    continue;
                }

                Time first_train_start_time =
                    make_time(first_start_date, first_ref.train_start_time);
                Time first_leaving_time =
                    first_train_start_time + first_ref.leave_offset;
                Time transfer_arriving_time =
                    first_train_start_time + first_ref.arrive_offset;
                int first_price = first_ref.price;
                Duration first_duration =
                    transfer_arriving_time - first_leaving_time;

                if (sorting_policy == "cost" &&
                    best_first_price_so_far < first_price) {
                    continue;
                }
                if (sorting_policy == "cost" &&
                    first_price < best_first_price_so_far) {
                    best_first_price_so_far = first_price;
                }

                if (has_result) {
                    if (sorting_policy == "cost") {
                        if (first_price >= best_price) {
                            continue;
                        }
                    } else if (first_duration >= best_duration) {
                        continue;
                    }
                }

                TicketResult cur_first = {
                    .trainID = first_ref.trainID,
                    .from = from,
                    .to = transfer_station,
                    .leaving_time = first_leaving_time,
                    .arriving_time = transfer_arriving_time,
                    .price = first_price,
                    .seat = 0};

                for (const TrainRef& second_ref : second_refs) {
                    if (second_ref.trainID == first_ref.trainID) {
                        continue;
                    }

                    int second_from_idx = second_ref.from_idx;
                    int second_to_idx = second_ref.to_idx;

                    Duration second_depart_offset =
                        second_ref.train_start_time + second_ref.leave_offset;
                    Date second_start_date =
                        (transfer_arriving_time - second_depart_offset + 1439) /
                        1440;
                    if (second_start_date < second_ref.sale_date[0]) {
                        second_start_date = second_ref.sale_date[0];
                    }
                    if (!in_range(second_ref.sale_date[0],
                                  second_ref.sale_date[1], second_start_date)) {
                        continue;
                    }

                    Time second_train_start_time = make_time(
                        second_start_date, second_ref.train_start_time);
                    TicketResult cur_second = {
                        .trainID = second_ref.trainID,
                        .from = transfer_station,
                        .to = to,
                        .leaving_time =
                            second_train_start_time + second_ref.leave_offset,
                        .arriving_time =
                            second_train_start_time + second_ref.arrive_offset,
                        .price = second_ref.price,
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
        if (!train.released) {
            return false;
        }
        int from_id = -1, to_id = -1;
        if (!get_station_id(from, from_id) || !get_station_id(to, to_id)) {
            return false;
        }
        int from_idx = static_cast<int>(
            find(train.station_ids, train.station_ids + train.station_num,
                 from_id) -
            train.station_ids);
        if (from_idx == train.station_num) {
            return false;
        }
        if (num > train.seat_num) {
            return false;
        }
        int to_idx = static_cast<int>(
            find(train.station_ids + from_idx + 1,
                 train.station_ids + train.station_num, to_id) -
            train.station_ids);
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
