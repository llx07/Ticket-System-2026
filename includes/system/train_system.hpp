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
        MinuteOfDay start_time;
        Duration travel_times[MAX_STATION - 1];
        Duration stopover_times[MAX_STATION - 2];
        Day sale_date[2];
        char type;
        bool released;
    };

   private:
    MemoryRiver<Train, 0> trains_dat;
    BPlusTree<TrainID, int> train_index;

    bool get_train(const TrainID& trainID, Train& train, int& idx) {
        if (!train_index.find(trainID, idx)) {
            return false;
        }
        trains_dat.read(train, idx);
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
        : trains_dat("trains.dat"), train_index("train_index.idx") {}

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
        train.start_time = parse_minute_of_day(start_time_str);
        train.type = type;
        train.released = false;

        auto stations = split(stations_str, '|');
        for (int i = 0; i < station_num; ++i) {
            train.stations[i] = stations[i];
        }

        parse_int_list(prices_str, train.prices, station_num - 1);
        parse_int_list(travel_times_str, train.travel_times, station_num - 1);
        if (station_num > 2) {
            parse_int_list(stopover_times_str, train.stopover_times,
                           station_num - 2);
        }

        auto sale_dates = split(sale_date_str, '|');
        train.sale_date[0] = parse_date(sale_dates[0]);
        train.sale_date[1] = parse_date(sale_dates[1]);

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

        train.released = true;
        trains_dat.update(train, idx);
        return true;
    }
};

#endif  // TICKET_SYSTEM_TRAIN_SYSTEM_HPP
