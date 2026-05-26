#ifndef TICKET_SYSTEM_DATE_TIME_HPP
#define TICKET_SYSTEM_DATE_TIME_HPP

#include <string>

using Date = int;          // 一个特定日期，(01-01 = 0)
using Time = int;          // 从 01-01 00:00 开始的分钟数
using Duration = int;      // 持续分钟数
using MinuteOfDate = int;  // 一天的某一个时刻

inline int month_dates(int month) {
    static int dates[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return dates[month - 1];
}

inline std::string two_digits(int value) {
    std::string result;
    result += static_cast<char>('0' + value / 10);
    result += static_cast<char>('0' + value % 10);
    return result;
}

// mm-dd -> Date
inline Date parse_date(const std::string& str) {
    const int month = (str[0] - '0') * 10 + (str[1] - '0');
    const int date = (str[3] - '0') * 10 + (str[4] - '0');

    Date result = 0;
    for (int m = 1; m < month; ++m) {
        result += month_dates(m);
    }
    return result + date - 1;
}

// hh:mm -> MinuteOfDate
inline MinuteOfDate parse_minute_of_date(const std::string& str) {
    const int hour = (str[0] - '0') * 10 + (str[1] - '0');
    const int minute = (str[3] - '0') * 10 + (str[4] - '0');
    return hour * 60 + minute;
}

inline Time make_time(Date date, MinuteOfDate minute_of_date) {
    return date * 1440 + minute_of_date;
}

inline Date get_date(Time time) { return time / 1440; }

inline MinuteOfDate get_minute_of_date(Time time) { return time % 1440; }

inline std::string format_date(Date date) {
    int month = 1;
    int d = date + 1;

    while (d > month_dates(month)) {
        d -= month_dates(month);
        ++month;
    }

    return two_digits(month) + "-" + two_digits(d);
}

inline std::string format_minute_of_date(MinuteOfDate minute_of_date) {
    return two_digits(minute_of_date / 60) + ":" +
           two_digits(minute_of_date % 60);
}

inline std::string format_time(Time time) {
    return format_date(get_date(time)) + " " +
           format_minute_of_date(get_minute_of_date(time));
}

inline bool in_range(Date start_date, Date end_date, Date cur_date) {
    return start_date <= cur_date && cur_date <= end_date;
}

#endif  // TICKET_SYSTEM_DATE_TIME_HPP
