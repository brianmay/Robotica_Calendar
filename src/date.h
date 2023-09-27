#ifndef DATE_H
#define DATE_H

#include <ostream>

#include "datetime.h"
#include "types.h"
#include "mystring.h"

namespace Project
{
    class Date
    {
    public:
        Date();
        Date(days_t index);
        Date(const string &date);
        Date(unsigned year, unsigned month, unsigned day);
        Date(const DateTime &datetime);

        void str(ostream &out) const;
        string as_str() const;

        unsigned year;
        unsigned month;
        unsigned day;

        bool valid() const;

        DateTime::Day getDayOfWeek() const;
        unsigned weekNo() const;
        unsigned dayOfYear() const;
        unsigned daysInMonth() const;
        unsigned daysInYear() const;

        void incYear(unsigned n);
        void incMonth(unsigned n);
        void incWeek(unsigned n, DateTime::Day wkst);
        void incDay(unsigned n);

        void decDay(unsigned n);
        void decMonth(unsigned n);

        void setWeekNo(unsigned n);

        Date(const Date &) = default;
        Date &operator=(const Date &ds);

        bool operator>(const Date &ds) const;
        bool operator<(const Date &ds) const;
        bool operator<=(const Date &ds) const;
        bool operator>=(const Date &ds) const;
        bool operator==(const Date &ds) const;
        bool operator!=(const Date &ds) const;

        unsigned operator-(const Date &other) const;
        Date operator+(const int days) const;

        DateTime start_of_day(TZ_ptr tz) const;

        string format(string format) const;

    protected:
        days_t index() const;
        void validate() const;
        DateTime::Day getWeekDay(unsigned days) const;
    };
}

#endif