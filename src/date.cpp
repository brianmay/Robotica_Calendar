/*############################################################################
# Copyright (c) 2020 Source Simian  :  https://github.com/sourcesimian/uICAL #
############################################################################*/
#include "date.h"

#include <tuple>

#include "error.h"
#include "datecalc.h"
#include "string.h"
#include "mytime.h"
#include "tz.h"

namespace Project
{
    Date::Date()
    {
        this->year = 1970;
        this->month = 1;
        this->day = 1;
        this->validate();
    }

    Date::Date(days_t days)
    {
        std::tuple<unsigned, unsigned, unsigned> ymd = civil_from_days(days);
        this->year = std::get<0>(ymd);
        this->month = std::get<1>(ymd);
        this->day = std::get<2>(ymd);
        this->validate();
    }

    Date::Date(const string &date)
    {
        for (;;)
        {
            try
            {
                if (date.length() != 8)
                {
                    break;
                }
                this->year = date.substr(0, 4).as_int();
                this->month = date.substr(4, 2).as_int();
                this->day = date.substr(6, 2).as_int();
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
        throw ValueError(string("Bad date: \"") + date + "\"");
    }

    Date::Date(unsigned year, unsigned month, unsigned day)
    {
        this->year = year;
        this->month = month;
        this->day = day;
        this->validate();
    }

    Date::Date(const DateTime &datetime)
    {
        EpochTime::ymdhms_t ymdhms = datetime.epoch_time.ymdhms(datetime.tz);
        this->year = std::get<0>(ymdhms);
        this->month = std::get<1>(ymdhms);
        this->day = std::get<2>(ymdhms);
        this->validate();
    }

    void Date::validate() const
    {
        ostream m;
        for (;;)
        {
            m << "Invalid ";
            if (this->year < 1970)
            {
                m << "year: " << year;
                break;
            }
            if (this->month < 1 || this->month > 12)
            {
                m << "month: " << month;
                break;
            }
            if (this->day < 1 || this->day > 31)
            {
                m << "day: " << day;
                break;
            }
            return;
        }
        throw ValueError(m);
    }

    bool Date::valid() const
    {
        return (this->year + this->month + this->day);
    }

    Date &Date::operator=(const Date &ds)
    {
        this->year = ds.year;
        this->month = ds.month;
        this->day = ds.day;
        return *this;
    }

    bool Date::operator>(const Date &ds) const
    {
        return this->index() > ds.index();
    }

    bool Date::operator<(const Date &ds) const
    {
        return this->index() < ds.index();
    }

    bool Date::operator<=(const Date &ds) const
    {
        return this->index() <= ds.index();
    }

    bool Date::operator>=(const Date &ds) const
    {
        return this->index() >= ds.index();
    }

    bool Date::operator==(const Date &ds) const
    {
        return this->index() == ds.index();
    }

    bool Date::operator!=(const Date &ds) const
    {
        return this->index() != ds.index();
    }

    unsigned Date::operator-(const Date &other) const
    {
        return this->index() - other.index();
    }

    Date Date::operator+(const int days) const
    {
        return Date(this->index() + days);
    }

    DateTime Date::start_of_day(TZ_ptr tz) const
    {
        return DateTime(*this, Time(0, 0, 0), tz);
    }

    days_t Date::index() const
    {
        return days_from_civil(this->year, this->month, this->day);
    }

    void Date::str(ostream &out) const
    {
        this->year < 9999 ? out << string::fmt(fmt_04d, this->year) : out << "????";
        this->month > 0 && this->month < 13 ? out << string::fmt(fmt_02d, this->month) : out << "??";
        this->day > 0 && this->day < 32 ? out << string::fmt(fmt_02d, this->day) : out << "??";
    }

    string Date::as_str() const
    {
        ostream stm;
        this->str(stm);
        return stm;
    }

    DateTime::Day Date::getDayOfWeek() const
    {
        auto days = days_from_civil(this->year, this->month, this->day);
        return Date::getWeekDay(days);
    }

    DateTime::Day Date::getWeekDay(unsigned days) const
    {
        unsigned weekday = weekday_from_days(days);
        if (weekday == 0)
            weekday = 7;
        return (DateTime::Day)weekday;
    }

    unsigned Date::weekNo() const
    {
        auto getWeekNo = [](unsigned weekdayFirst, unsigned yearDayIndex)
        {
            if (weekdayFirst <= 4)
                return (weekdayFirst + yearDayIndex - 1) / 7 + 1;
            else
                return (weekdayFirst + yearDayIndex - 1) / 7;
        };

        unsigned days = days_from_civil(this->year, this->month, this->day);
        unsigned daysFirst = days_from_civil(this->year, 1, 1);
        unsigned weekdayFirst = (unsigned)this->getWeekDay(daysFirst);
        unsigned yearDayIndex = days - daysFirst;

        unsigned weekNo = getWeekNo(weekdayFirst, yearDayIndex);
        if (weekNo != 0)
            return weekNo;

        if (is_leap(this->year - 1))
            daysFirst -= 366;
        else
            daysFirst -= 365;

        weekdayFirst = (unsigned)this->getWeekDay(daysFirst);
        yearDayIndex = days - daysFirst;

        return getWeekNo(weekdayFirst, yearDayIndex);
    }

    unsigned Date::dayOfYear() const
    {
        return days_from_civil(this->year, this->month, this->day) - days_from_civil(this->year, 1, 1) + 1;
    }

    unsigned Date::daysInMonth() const
    {
        return last_day_of_month(this->year, this->month);
    }

    unsigned Date::daysInYear() const
    {
        if (is_leap(this->year))
        {
            return 366;
        }
        return 365;
    }

    void Date::setWeekNo(unsigned n)
    {
        auto yearDayIndex = [](unsigned weekdayFirst, unsigned weekNo)
        {
            if (weekdayFirst <= 4)
                return ((int)weekNo - 1) * 7 - (int)weekdayFirst + 1;
            else
                return (int)weekNo * 7 - (int)weekdayFirst + 1;
        };

        unsigned daysFirst = days_from_civil(this->year, 1, 1);
        unsigned weekdayFirst = (unsigned)this->getWeekDay(daysFirst);

        this->day = 1;
        this->month = 1;
        int index = yearDayIndex(weekdayFirst, n);
        if (index > 0)
            this->incDay(index);
    }

    void Date::decDay(unsigned n)
    {
        this->day--;
        while (n > this->day)
        {
            n -= this->day;
            this->decMonth(1);
            this->day = last_day_of_month(this->year, this->month);
        }
        this->day -= n;
        this->day++;
    }

    void Date::incDay(unsigned n)
    {
        this->day--;
        {
            this->day += n;
            while (true)
            {
                if (this->day < 27)
                {
                    break;
                }
                unsigned last_day = last_day_of_month(this->year, this->month);
                if (this->day < last_day)
                {
                    break;
                }
                this->day -= last_day;
                this->incMonth(1);
            }
        }
        this->day++;
    }

    void Date::incWeek(unsigned n, DateTime::Day wkst)
    {
        auto dayOfWeek = this->getDayOfWeek();
        this->incDay(DateTime::daysUntil(dayOfWeek, wkst));
        this->incDay(7 * (n - (dayOfWeek == wkst ? 0 : 1)));
    }

    void Date::decMonth(unsigned n)
    {
        this->month--;
        while (n > this->month)
        {
            n -= month;
            this->month = 11;
            this->year--;
        }
        this->month -= n;
        this->month++;
    }

    void Date::incMonth(unsigned n)
    {
        this->month--;
        {
            this->month += n;
            if (this->month > 11)
            {
                this->incYear(this->month / 12);
            }
            this->month %= 12;
        }
        this->month++;
    }

    void Date::incYear(unsigned n)
    {
        this->year += n;
    }

    string Date::format(string format) const
    {
        DateTime datetime = this->start_of_day(tz_unaware);
        return datetime.format(format);
    }
}
