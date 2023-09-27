#include "mytime.h"

#include "epochtime.h"
#include "error.h"
#include "datecalc.h"
#include "datetime.h"

namespace Project
{
    Time::Time()
    {
        this->hour = 0;
        this->minute = 0;
        this->second = 0;
    }

    Time::Time(const string &time)
    {
        for (;;)
        {
            try
            {
                if (time.length() != 6)
                {
                    break;
                }
                this->hour = time.substr(9, 2).as_int();
                this->minute = time.substr(11, 2).as_int();
                this->second = time.substr(13, 2).as_int();
                this->validate();
                return;
            }
            catch (std::invalid_argument const &e)
            {
            }
            catch (std::out_of_range const &e)
            {
            }
        }
        throw ValueError(string("Bad time: \"") + time + "\"");
    }

    Time::Time(unsigned hour, unsigned minute, unsigned second)
    {
        this->hour = hour;
        this->minute = minute;
        this->second = second;
        this->validate();
    }

    Time::Time(const DateTime &datetime)
    {
        EpochTime::ymdhms_t ymdhms = datetime.epoch_time.ymdhms(datetime.tz);
        this->hour = std::get<3>(ymdhms);
        this->minute = std::get<4>(ymdhms);
        this->second = std::get<5>(ymdhms);
        this->validate();
    }

    void Time::validate() const
    {
        ostream m;
        for (;;)
        {
            m << "Invalid ";
            if (this->hour > 23)
            {
                m << "hour: " << hour;
                break;
            }
            if (this->minute > 59)
            {
                m << "minute: " << minute;
                break;
            }
            if (this->second > 59)
            {
                m << "second: " << second;
                break;
            }
            return;
        }
        throw ValueError(m);
    }

    bool Time::valid() const
    {
        return (this->hour + this->minute + this->second);
    }

    Time &Time::operator=(const Time &ds)
    {
        this->hour = ds.hour;
        this->minute = ds.minute;
        this->second = ds.second;
        return *this;
    }

    bool Time::operator>(const Time &ds) const
    {
        return this->index() > ds.index();
    }

    bool Time::operator<(const Time &ds) const
    {
        return this->index() < ds.index();
    }

    bool Time::operator<=(const Time &ds) const
    {
        return this->index() <= ds.index();
    }

    bool Time::operator==(const Time &ds) const
    {
        return this->index() == ds.index();
    }

    bool Time::operator!=(const Time &ds) const
    {
        return this->index() != ds.index();
    }

    seconds_t Time::index() const
    {
        return ((this->hour * 60) + this->minute * 60) + this->second;
    }

    void Time::str(ostream &out) const
    {
        this->hour < 24 ? out << string::fmt(fmt_02d, this->hour) : out << "??";
        this->minute < 60 ? out << string::fmt(fmt_02d, this->minute) : out << "??";
        ;
        this->second < 60 ? out << string::fmt(fmt_02d, this->second) : out << "??";
        ;
    }

    string Time::as_str() const
    {
        ostream stm;
        this->str(stm);
        return stm;
    }

    void Time::incSecond(unsigned n)
    {
        this->second += n;
        if (this->second > 59)
        {
            this->incMinute(this->second / 60);
            this->second %= 60;
        }
    }

    void Time::incMinute(unsigned n)
    {
        this->minute += n;
        if (this->minute > 59)
        {
            this->incHour(this->minute / 60);
            this->minute %= 60;
        }
    }

    void Time::incHour(unsigned n)
    {
        this->hour += n;
        if (this->hour > 23)
        {
            ValueError(string("Hour exceeds 23 after increment"));
        }
    }
}