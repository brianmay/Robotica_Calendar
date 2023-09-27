/*############################################################################
# Copyright (c) 2020 Source Simian  :  https://github.com/sourcesimian/uICAL #
############################################################################*/
#include "local_time.h"

namespace Project
{
    Local_TZ::Local_TZ(Timezone_ptr timezone)
    {
        this->timezone = timezone;
    }

    seconds_t Local_TZ::toUTC(seconds_t timestamp) const
    {
        return this->timezone->toUTC(timestamp);
    }

    seconds_t Local_TZ::fromUTC(seconds_t timestamp) const
    {
        return this->timezone->toLocal(timestamp);
    }

    void Local_TZ::str(ostream &out) const
    {
        out << "LocalTZ";
    }

    void Local_TZ::output_details(ostream &out) const
    {
        out << "LocalTZ";
    }
}