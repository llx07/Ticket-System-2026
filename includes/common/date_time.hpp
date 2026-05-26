#ifndef TICKET_SYSTEM_DATE_TIME_HPP
#define TICKET_SYSTEM_DATE_TIME_HPP

#include <string>

using Day = int;          // 一个特定日期，(06-01 = 0)
using Time = int;         // 从 06-01 00:00 开始的分钟数
using Duration = int;     // 持续分钟数
using MinuteOfDay = int;  // 一天的某一个时刻

inline int month_days(int month) {
    static int days[] = {30, 31, 31};
    return days[month - 6];
}

inline std::string two_digits(int value) {
    std::string result;
    result += static_cast<char>('0' + value / 10);
    result += static_cast<char>('0' + value % 10);
    return result;
}

// mm-dd -> Day
inline Day parse_date(const std::string& str) {
    const int month = (str[0] - '0') * 10 + (str[1] - '0');
    const int day = (str[3] - '0') * 10 + (str[4] - '0');

    Day result = 0;
    for (int m = 6; m < month; ++m) {
        result += month_days(m);
    }
    return result + day - 1;
}

// hh:mm -> MinuteOfDay
inline MinuteOfDay parse_minute_of_day(const std::string& str) {
    const int hour = (str[0] - '0') * 10 + (str[1] - '0');
    const int minute = (str[3] - '0') * 10 + (str[4] - '0');
    return hour * 60 + minute;
}

inline Time make_time(Day day, MinuteOfDay minute_of_day) {
    return day * 1440 + minute_of_day;
}

inline Day get_day(Time time) { return time / 1440; }

inline MinuteOfDay get_minute_of_day(Time time) { return time % 1440; }

inline std::string format_date(Day day) {
    int month = 6;
    int date = day + 1;

    while (date > month_days(month)) {
        date -= month_days(month);
        ++month;
    }

    return two_digits(month) + "-" + two_digits(date);
}

inline std::string format_minute_of_day(MinuteOfDay minute_of_day) {
    return two_digits(minute_of_day / 60) + ":" +
           two_digits(minute_of_day % 60);
}

inline std::string format_time(Time time) {
    return format_date(get_day(time)) + " " +
           format_minute_of_day(get_minute_of_day(time));
}

#endif  // TICKET_SYSTEM_DATE_TIME_HPP
