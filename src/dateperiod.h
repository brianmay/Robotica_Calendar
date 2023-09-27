#ifndef dateperiod_h
#define dateperiod_h

#include "mystring.h"
#include "stream.h"
#include "types.h"

namespace Project
{
    class DatePeriod
    {
    public:
        DatePeriod();
        DatePeriod(seconds_t span);
        DatePeriod(unsigned days, unsigned hours, unsigned minutes, unsigned seconds);

        seconds_t totalSeconds() const;

        void str(ostream &out) const;
        string as_str() const;

    protected:
        seconds_t spanSeconds;
    };
}

#endif
