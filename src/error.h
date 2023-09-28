#ifndef ERROR_H
#define ERROR_H

#include "mystring.h"

namespace Project
{
    class Error
    {
    public:
        string message;
    };

    class ParseError : public Error
    {
    public:
        ParseError(string m)
        {
            this->message = string("ParseError: ") + m;
        }
    };

    class ValueError : public Error
    {
    public:
        ValueError(string m)
        {
            this->message = string("ValueError: ") + m;
        }
    };

    class NotImplementedError : public Error
    {
    public:
        NotImplementedError(string m)
        {
            this->message = string("NotImplementedError: ") + m;
        }
    };

    class ImplementationError : public Error
    {
    public:
        ImplementationError(string m)
        {
            this->message = string("ImplementationError: ") + m;
        }
    };

    class ICalError : public Error
    {
    public:
        ICalError(string m)
        {
            this->message = string("ICalError: ") + m;
        }
    };

    class RecurrenceError : public Error
    {
    public:
        RecurrenceError(string m)
        {
            this->message = string("RecurrenceError: ") + m;
        }
    };

    class TZAwarenessConflictError : public Error
    {
    public:
        TZAwarenessConflictError(string m)
        {
            this->message = string("TZAwarenessConflictError: ") + m;
        }
    };
}

#endif
