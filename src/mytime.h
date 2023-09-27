#ifndef time_h
#define time_h

#include "types.h"
#include "mystring.h"

namespace Project
{
    class Time
    {
    public:
        Time();
        Time(const string &time);
        Time(unsigned hour, unsigned minute, unsigned second);
        Time(const DateTime &datetime);

        void str(ostream &out) const;
        string as_str() const;

        unsigned hour;
        unsigned minute;
        unsigned second;

        bool valid() const;

        void incHour(unsigned n);
        void incMinute(unsigned n);
        void incSecond(unsigned n);

        Time(const Time &) = default;
        Time &operator=(const Time &ds);

        bool operator>(const Time &ds) const;
        bool operator<(const Time &ds) const;
        bool operator<=(const Time &ds) const;
        bool operator==(const Time &ds) const;
        bool operator!=(const Time &ds) const;

    protected:
        seconds_t index() const;
        void validate() const;
    };
}

#endif
