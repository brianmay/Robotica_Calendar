#ifndef DATETIME_H
#define DATETIME_H

#include "datecalc.h"
#include "dateperiod.h"
#include "epochtime.h"
#include "mystring.h"
#include "types.h"

namespace Project
{
    class TZ;

    struct Date;
    struct Time;

    class DateTime
    {
    public:
        DateTime();
        DateTime(const string &datetime, const TZ_ptr &tzm);
        DateTime(seconds_t epochSeconds, const TZ_ptr tz);
        DateTime(EpochTime epoch_time, const TZ_ptr tz);
        DateTime(const DateTime &) = default;
        DateTime(const Date &date, const Time &time, const TZ_ptr tz);
        static DateTime utc_now();
        static DateTime local_now(const TZ_ptr tz);

        void
        str(ostream &out) const;
        string as_str() const;
        string format(string format) const;
        dhms_t convert_to_dhms() const;

        bool valid() const;

        Date date() const;
        Time time() const;
        DateTime shift_timezone(const TZ_ptr tz) const;

        enum class Day
        {
            NONE,
            MON,
            TUE,
            WED,
            THU,
            FRI,
            SAT,
            SUN
        };

        static unsigned daysUntil(DateTime::Day today, DateTime::Day then);
        static unsigned daysUntil(DateTime::Day today, int index, DateTime::Day then, unsigned span);
        static DateTime::Day dayOfWeekAfter(DateTime::Day today, unsigned days);

        DateTime &operator=(const DateTime &dt);
        DatePeriod operator+(const DateTime &dt) const;
        DatePeriod operator-(const DateTime &dt) const;
        DateTime operator+(const DatePeriod &dp) const;
        DateTime operator-(const DatePeriod &dp) const;

        bool operator>(const DateTime &dt) const;
        bool operator<(const DateTime &dt) const;
        bool operator>=(const DateTime &dt) const;
        bool operator<=(const DateTime &dt) const;
        bool operator==(const DateTime &dt) const;

        TZ_ptr tz;
        EpochTime epoch_time;

    protected:
        void construct(const string &datetime, const TZ_ptr &tz);
        void assert_awareness(const DateTime &other) const;
    };
}

// namespace std
// {
//     template <>
//     struct hash<DateTime>
//     {
//         size_t operator()(const DateTime &k) const
//         {
//             return k.epoch_time.epochSeconds;
//         }
//     };
// }

#endif