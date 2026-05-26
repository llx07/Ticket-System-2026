#ifndef TICKET_SYSTEM_TRAIN_SYSTEM_HPP
#define TICKET_SYSTEM_TRAIN_SYSTEM_HPP

#include <string>

#include "common/date_time.hpp"
#include "common/fixed_string.hpp"
#include "common/util.hpp"
#include "storage/b_plus_tree.hpp"
#include "storage/memory_river.hpp"

class TrainSystem {
   private:
    static constexpr int MAX_STATION = 100;

   public:
    using TrainID = FixedString<21>;
    using Station = FixedString<31>;

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
    TrainSystem()
        : trains_dat("trains.dat"),
          train_index("train_index.idx"),
          train_seats_dat("train_seats.dat"),
          train_seat_index("train_seat.idx") {}

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
            train_seats_dat.write(train_seat);
            train_seat_index.insert({trainID, d}, idx);
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
};

#endif  // TICKET_SYSTEM_TRAIN_SYSTEM_HPP
