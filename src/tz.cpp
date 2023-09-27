#include "tz.h"

#include <ostream>

#include "error.h"
#include "types.h"

namespace Project
{
    const TZ_ptr tz_unaware = new_ptr<UnawareTZ>();
    const TZ_ptr tz_UTC = new_ptr<OffsetTZ>("Z", 0);

    string TZ::as_str() const
    {
        ostream stm;
        this->str(stm);
        return stm;
    }

    bool TZ::is_aware() const
    {
        return true;
    }

    UnawareTZ::UnawareTZ()
    {
    }

    seconds_t UnawareTZ::toUTC(seconds_t timestamp) const
    {
        return timestamp;
    }
    seconds_t UnawareTZ::fromUTC(seconds_t timestamp) const
    {
        return timestamp;
    }

    void UnawareTZ::str(ostream &out) const
    {
    }

    OffsetTZ::OffsetTZ(const string &name, int offsetMins)
    {
        this->name = name;
        this->offsetMins = offsetMins;
    }

    OffsetTZ::OffsetTZ(const string &name, const string &tz)
    {
        this->name = name;
        this->offsetMins = OffsetTZ::parseOffset(tz);
    }

    int OffsetTZ::parseOffset(const string &tz)
    {
        try
        {
            if (tz.length() == 5)
            {
                char sign;
                unsigned tzH, tzM;

                // e.g.: +0200
                sign = tz.at(0);
                tzH = tz.substr(1, 2).as_int();
                tzM = tz.substr(3, 2).as_int();

                int offset = (tzH * 60) + tzM;
                if (sign == '-')
                {
                    offset *= -1;
                }
                return offset;
            }
        }
        catch (std::invalid_argument const &e)
        {
        }
        catch (std::out_of_range const &e)
        {
        }
        throw ValueError("Bad timezone: \"" + tz + "\"");
    }

    void OffsetTZ::output_details(ostream &out) const
    {
        int offsetMins = this->offsetMins;
        if (offsetMins != -1)
        {
            if (offsetMins < 0)
            {
                out << "-";
                offsetMins *= -1;
            }
            else
            {
                out << "+";
            }
            out << string::fmt(fmt_02d, offsetMins / 60);
            out << string::fmt(fmt_02d, offsetMins % 60);
        }
    }

    int OffsetTZ::offset() const
    {
        if (this->offsetMins == -1)
            throw ImplementationError("Timezone not defined");
        return this->offsetMins;
    }

    seconds_t OffsetTZ::toUTC(seconds_t timestamp) const
    {
        return timestamp - (this->offset() * 60);
    }

    seconds_t OffsetTZ::fromUTC(seconds_t timestamp) const
    {
        return timestamp + (this->offset() * 60);
    }

    void OffsetTZ::str(ostream &out) const
    {
        out << this->name;
    }
}