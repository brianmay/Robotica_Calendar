#ifndef local_time_tz_h
#define local_time_tz_h

#include <memory>
#include <Timezone.h>

#include "types.h"
#include "stream.h"
#include "tz.h"
#include "local_time.h"

namespace Project
{
    class DateStamp;

    using Timezone_ptr = std::shared_ptr<Timezone>;

    class Local_TZ : public TZ
    {
    public:
        Local_TZ(const Timezone_ptr timezone);
        virtual seconds_t toUTC(seconds_t timestamp) const;
        virtual seconds_t fromUTC(seconds_t timestamp) const;
        virtual void str(ostream &out) const;
        virtual void output_details(ostream &out) const;

    private:
        Timezone_ptr timezone;
    };

    using Local_TZ_ptr = std::shared_ptr<Local_TZ>;
}
#endif
