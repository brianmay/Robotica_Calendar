#ifndef TYPES_H
#define TYPES_H

#include <memory>

namespace Project
{
    using seconds_t = long long int;
    using days_t = long long int;

    template <typename T, typename... Args>
    inline std::shared_ptr<T> new_ptr(Args... args) { return std::shared_ptr<T>(new T(args...)); }

#define PTR(T) \
    class T;   \
    using T##_ptr = std::shared_ptr<T>;

    PTR(DateTime)
    PTR(TZ)
}

#endif