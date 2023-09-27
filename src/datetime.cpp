#include "datetime.h"

#include <stdio.h>
#include <time.h>

#include "error.h"
#include "date.h"
#include "datetime.h"
#include "epochtime.h"
#include "mytime.h"
#include "tz.h"

namespace Project
{
    DateTime::DateTime()
    {
        this->epoch_time = 0;
        this->tz = tz_UTC;
    }

    DateTime::DateTime(const string &datetime, const TZ_ptr &tz)
    {
        this->construct(datetime, tz);
    }

    DateTime::DateTime(seconds_t epochSeconds, const TZ_ptr tz)
    {
        this->epoch_time = EpochTime(epochSeconds);
        this->tz = tz;
    }

    DateTime::DateTime(EpochTime epoch_time, const TZ_ptr tz)
    {
        this->epoch_time = epoch_time;
        this->tz = tz;
    }

    DateTime::DateTime(const Date &date, const Time &time, const TZ_ptr tz)
    {
        this->epoch_time = EpochTime(
            date.year, date.month, date.day, time.hour, time.minute, time.second,
            tz);
        this->tz = tz;
    }

    DateTime DateTime::utc_now()
    {
        return DateTime(EpochTime::utc_now(), tz_UTC);
    }

    DateTime DateTime::local_now(const TZ_ptr tz)
    {
        return DateTime::utc_now().shift_timezone(tz);
    }

    void DateTime::construct(const string &datetime, const TZ_ptr &tz)
    {
        tm timeinfo;
        strptime(datetime.c_str(), "%FT%TZ", &timeinfo);
        seconds_t time = mktime(&timeinfo);
        this->epoch_time = EpochTime(time);
        this->tz = tz;
    }

    bool DateTime::valid() const
    {
        return this->epoch_time.valid();
    }

    DateTime DateTime::shift_timezone(const TZ_ptr tz) const
    {
        return DateTime(this->epoch_time, tz);
    }

    Date DateTime::date() const { return Date(*this); };
    Time DateTime::time() const { return Time(*this); };

    DateTime &DateTime::operator=(const DateTime &other)
    {
        this->tz = other.tz;
        this->epoch_time = other.epoch_time;
        return *this;
    }

    void DateTime::assert_awareness(const DateTime &other) const
    {
        if (this->tz->is_aware() != other.tz->is_aware())
        {
            throw TZAwarenessConflictError(this->as_str() + this->tz->as_str() + " <> " + other.as_str() + other.tz->as_str());
        }
    }

    DatePeriod DateTime::operator-(const DateTime &other) const
    {
        this->assert_awareness(other);
        return DatePeriod(this->epoch_time.epochSeconds - other.epoch_time.epochSeconds);
    }

    DatePeriod DateTime::operator+(const DateTime &other) const
    {
        this->assert_awareness(other);
        return DatePeriod(this->epoch_time.epochSeconds + other.epoch_time.epochSeconds);
    }

    DateTime DateTime::operator+(const DatePeriod &dp) const
    {
        return DateTime(this->epoch_time.epochSeconds + dp.totalSeconds(), this->tz);
    }

    DateTime DateTime::operator-(const DatePeriod &dp) const
    {
        return DateTime(this->epoch_time.epochSeconds - dp.totalSeconds(), this->tz);
    }

    bool DateTime::operator>(const DateTime &other) const
    {
        this->assert_awareness(other);
        return this->epoch_time > other.epoch_time;
    }

    bool DateTime::operator<(const DateTime &other) const
    {
        this->assert_awareness(other);
        return this->epoch_time < other.epoch_time;
    }

    bool DateTime::operator>=(const DateTime &other) const
    {
        this->assert_awareness(other);
        return this->epoch_time >= other.epoch_time;
    }

    bool DateTime::operator<=(const DateTime &other) const
    {
        this->assert_awareness(other);
        return this->epoch_time <= other.epoch_time;
    }

    bool DateTime::operator==(const DateTime &other) const
    {
        this->assert_awareness(other);
        return this->epoch_time == other.epoch_time;
    }

    void DateTime::str(ostream &out) const
    {
        auto ymdhms = this->epoch_time.ymdhms(this->tz);

        out << string::fmt(fmt_04d, std::get<0>(ymdhms));
        out << string::fmt(fmt_02d, std::get<1>(ymdhms));
        out << string::fmt(fmt_02d, std::get<2>(ymdhms));
        out << "T";
        out << string::fmt(fmt_02d, std::get<3>(ymdhms));
        out << string::fmt(fmt_02d, std::get<4>(ymdhms));
        out << string::fmt(fmt_02d, std::get<5>(ymdhms));

        this->tz->str(out);
    }

    string DateTime::as_str() const
    {
        ostream stm;
        this->str(stm);
        return stm;
    }

    string DateTime::format(string format) const
    {
        const time_t secs = this->tz->fromUTC(this->epoch_time.epochSeconds);
        const struct tm time = *gmtime(&secs);
        char buffer[64];
        strftime(buffer, 64, format.c_str(), &time);
        string result = buffer;
        return result;
    }

    dhms_t DateTime::convert_to_dhms() const
    {
        return to_dhms(this->epoch_time.epochSeconds);
    }

    unsigned DateTime::daysUntil(DateTime::Day today, DateTime::Day then)
    {
        return ((int)today <= (int)then ? 0 : 7) + (int)then - (int)today;
    }

    unsigned DateTime::daysUntil(DateTime::Day today, int index, DateTime::Day then, unsigned span)
    {
        if (index > 0)
        {
            span = 0;
        }
        else
        {
            index++;
            today = (DateTime::Day)((((int)today - 1 + span) % 7) + 1);
        }

        return span +
               ((index - ((unsigned)then >= (unsigned)today ? 1 : 0)) * 7) +
               ((unsigned)then - (unsigned)today);
    }

    DateTime::Day DateTime::dayOfWeekAfter(DateTime::Day today, unsigned days)
    {
        return DateTime::Day(((int)today + days - 1) % 7 + 1);
    }

    ostream &operator<<(ostream &out, const DateTime::Day &day)
    {
        if (day == DateTime::Day::MON)
            out << "MO";
        else if (day == DateTime::Day::TUE)
            out << "TU";
        else if (day == DateTime::Day::WED)
            out << "WE";
        else if (day == DateTime::Day::THU)
            out << "TH";
        else if (day == DateTime::Day::FRI)
            out << "FR";
        else if (day == DateTime::Day::SAT)
            out << "SA";
        else if (day == DateTime::Day::SUN)
            out << "SU";
        else
            out << "??";
        return out;
    }
}