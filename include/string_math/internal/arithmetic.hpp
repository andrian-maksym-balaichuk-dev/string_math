#pragma once
// Do not include this file directly. Use <string_math/string_math.hpp> or individual public headers.

#include <cmath>
#include <limits>
#include <stdexcept>

#include <string_math/value/value.hpp>
#include <string_math/value/policy.hpp>

namespace string_math::internal
{

inline long double factorial_value(long double value)
{
    if (value < 0.0L)
    {
        throw std::domain_error("string_math: factorial requires a non-negative value");
    }

    const long double rounded = std::round(value);
    if (std::fabs(static_cast<double>(rounded - value)) > 1.0e-9)
    {
        throw std::domain_error("string_math: factorial requires an integral value");
    }

    long double result = 1.0L;
    for (long long index = 2; index <= static_cast<long long>(rounded); ++index)
    {
        result *= static_cast<long double>(index);
    }
    return result;
}

template <class T>
constexpr bool m_add_overflow(T left, T right, T& output)
{
    if constexpr (std::is_unsigned_v<T>)
    {
        output = static_cast<T>(left + right);
        return output < left;
    }
    else
    {
        if ((right > 0 && left > std::numeric_limits<T>::max() - right) ||
            (right < 0 && left < std::numeric_limits<T>::lowest() - right))
        {
            return true;
        }
        output = static_cast<T>(left + right);
        return false;
    }
}

template <class T>
constexpr bool m_sub_overflow(T left, T right, T& output)
{
    if constexpr (std::is_unsigned_v<T>)
    {
        output = static_cast<T>(left - right);
        return left < right;
    }
    else
    {
        if ((right < 0 && left > std::numeric_limits<T>::max() + right) ||
            (right > 0 && left < std::numeric_limits<T>::lowest() + right))
        {
            return true;
        }
        output = static_cast<T>(left - right);
        return false;
    }
}

template <class T>
inline bool m_mul_overflow(T left, T right, T& output)
{
    const long double product = static_cast<long double>(left) * static_cast<long double>(right);
    if (product < static_cast<long double>(std::numeric_limits<T>::lowest()) ||
        product > static_cast<long double>(std::numeric_limits<T>::max()))
    {
        return true;
    }
    output = static_cast<T>(left * right);
    return false;
}

template <class T>
constexpr T m_saturate_cast(long double value)
{
    if (value < static_cast<long double>(std::numeric_limits<T>::lowest()))
    {
        return std::numeric_limits<T>::lowest();
    }
    if (value > static_cast<long double>(std::numeric_limits<T>::max()))
    {
        return std::numeric_limits<T>::max();
    }
    return static_cast<T>(value);
}

} // namespace string_math::internal
