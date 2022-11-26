/*############################################################################
# Copyright (c) 2020 Source Simian  :  https://github.com/sourcesimian/uICAL #
############################################################################*/
#include "local_time.h"

Local_TZ::Local_TZ(Timezone_ptr timezone)
{
    this->timezone = timezone;
}

uICAL::seconds_t Local_TZ::toUTC(uICAL::seconds_t timestamp) const
{
    return this->timezone->toUTC(timestamp);
}

uICAL::seconds_t Local_TZ::fromUTC(uICAL::seconds_t timestamp) const
{
    return this->timezone->toLocal(timestamp);
}

void Local_TZ::str(uICAL::ostream &out) const
{
    out << "LocalTZ";
}

void Local_TZ::output_details(uICAL::ostream &out) const
{
    out << "LocalTZ";
}
