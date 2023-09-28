#include "epochtime.h"

#include <ostream>

#include "datecalc.h"
#include "types.h"
#include "tz.h"

namespace Project
{

    const seconds_t EpochTime::NaN = (unsigned)-1;

    EpochTime::EpochTime()
    {
        this->epochSeconds = NaN;
    }

    EpochTime::EpochTime(unsigned year, unsigned month, unsigned day, unsigned hour, unsigned minute, unsigned second, const TZ_ptr tz)
    {
        auto epochDays = days_from_civil(year, month, day);
        this->epochSeconds = tz->toUTC(to_seconds(epochDays, hour, minute, second));
    }

    EpochTime::EpochTime(seconds_t seconds)
    {
        this->epochSeconds = seconds;
    }

    EpochTime EpochTime::utc_now()
    {
        return time(nullptr);
    }

    bool EpochTime::valid() const
    {
        return this->epochSeconds != NaN;
    }

    EpochTime::ymd_t EpochTime::ymd(const TZ_ptr tz) const
    {
        unsigned seconds = tz->fromUTC(this->epochSeconds);
        auto ymd = civil_from_days(seconds / (24 * 60 * 60));

        return ymd_t(std::get<0>(ymd), std::get<1>(ymd), std::get<2>(ymd));
    }

    EpochTime::ymdhms_t EpochTime::ymdhms(const TZ_ptr tz) const
    {
        seconds_t seconds = tz->fromUTC(this->epochSeconds);
        auto dhms = to_dhms(seconds);

        auto ymd = civil_from_days(std::get<0>(dhms));

        return ymdhms_t(
            std::get<0>(ymd), std::get<1>(ymd), std::get<2>(ymd),
            std::get<1>(dhms), std::get<2>(dhms), std::get<3>(dhms));
    }

    seconds_t EpochTime::operator-(const EpochTime &other) const
    {
        return this->epochSeconds - other.epochSeconds;
    }

    bool EpochTime::operator>(const EpochTime &other) const
    {
        return this->epochSeconds > other.epochSeconds;
    }

    bool EpochTime::operator<(const EpochTime &other) const
    {
        return this->epochSeconds < other.epochSeconds;
    }

    bool EpochTime::operator>=(const EpochTime &other) const
    {
        return this->epochSeconds >= other.epochSeconds;
    }

    bool EpochTime::operator<=(const EpochTime &other) const
    {
        return this->epochSeconds <= other.epochSeconds;
    }

    bool EpochTime::operator==(const EpochTime &other) const
    {
        return this->epochSeconds == other.epochSeconds;
    }

    bool EpochTime::operator!=(const EpochTime &other) const
    {
        return this->epochSeconds != other.epochSeconds;
    }

    void EpochTime::str(ostream &out) const
    {
        out << this->epochSeconds;
    }
}