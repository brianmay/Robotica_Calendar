#ifndef TZ_H
#define TZ_H

#include "types.h"
#include "mystring.h"
#include "stream.h"

namespace Project
{
    class TZ
    {
    public:
        virtual seconds_t toUTC(seconds_t timestamp) const = 0;
        virtual seconds_t fromUTC(seconds_t timestamp) const = 0;
        virtual void output_details(ostream &out) const = 0;
        virtual void str(ostream &stm) const = 0;
        virtual string as_str() const;
        virtual bool is_aware() const;
    };

    class UnawareTZ : public TZ
    {
    public:
        UnawareTZ();
        virtual seconds_t toUTC(seconds_t timestamp) const;
        virtual seconds_t fromUTC(seconds_t timestamp) const;
        virtual void str(ostream &out) const;
        virtual void output_details(ostream &out) const {};
    };

    class OffsetTZ : public TZ
    {
    public:
        OffsetTZ(const string &name, int offsetMins);
        OffsetTZ(const string &name, const string &tz);

        virtual seconds_t toUTC(seconds_t timestamp) const;
        virtual seconds_t fromUTC(seconds_t timestamp) const;
        virtual void str(ostream &out) const;
        virtual void output_details(ostream &out) const;

        int offset() const;

        static int parseOffset(const string &offset);

    private:
        int offsetMins;
        string name;
    };

    extern const TZ_ptr tz_unaware;
    extern const TZ_ptr tz_UTC;
}

#endif
