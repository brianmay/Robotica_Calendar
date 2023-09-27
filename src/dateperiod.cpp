#include "dateperiod.h"

#include <tuple>

#include "datecalc.h"

namespace Project
{
    DatePeriod::DatePeriod()
    {
        this->spanSeconds = 0;
    }

    DatePeriod::DatePeriod(seconds_t seconds)
    {
        this->spanSeconds = seconds;
    }

    DatePeriod::DatePeriod(unsigned days, unsigned hours, unsigned minutes, unsigned seconds)
    {
        this->spanSeconds = days * 24 * 60 * 60 + hours * 60 * 60 + minutes * 60 + seconds;
    }

    seconds_t DatePeriod::totalSeconds() const
    {
        return this->spanSeconds;
    }

    void DatePeriod::str(ostream &out) const
    {
        auto dhms = to_dhms(this->spanSeconds);

        out << "P";
        if (std::get<0>(dhms))
            out << std::get<0>(dhms) << "D";
        out << "T";
        if (std::get<1>(dhms))
            out << std::get<1>(dhms) << "H";
        if (std::get<2>(dhms))
            out << std::get<2>(dhms) << "M";
        if (std::get<3>(dhms))
            out << std::get<3>(dhms) << "S";
    }

    string DatePeriod::as_str() const
    {
        ostream stm;
        this->str(stm);
        return stm;
    }
}