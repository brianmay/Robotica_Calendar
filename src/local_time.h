/*############################################################################
# Copyright (c) 2020 Source Simian  :  https://github.com/sourcesimian/uICAL #
############################################################################*/
#ifndef local_time_tz_h
#define local_time_tz_h

#include <memory>
#include <uICAL/base.h>
#include <uICAL/types.h>
#include <uICAL/stream.h>
#include <uICAL/tz.h>
#include <Timezone.h>
#include "local_time.h"

class DateStamp;

using Timezone_ptr = std::shared_ptr<Timezone>;

class Local_TZ : public uICAL::TZ {
    public:
        Local_TZ(const Timezone_ptr timezone);
        virtual uICAL::seconds_t toUTC(uICAL::seconds_t timestamp) const;
        virtual uICAL::seconds_t fromUTC(uICAL::seconds_t timestamp) const;
        virtual void str(uICAL::ostream& out) const;

    private:
        Timezone_ptr timezone;
    };

using Local_TZ_ptr = std::shared_ptr<Local_TZ>;

#endif
