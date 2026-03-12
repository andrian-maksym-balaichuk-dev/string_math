#pragma once

#include <cmath>
#include <type_traits>

#include <string_math/detail/overload.hpp>

namespace string_math::detail
{

constexpr bool builtin_truthy(const MathValue& value) noexcept
{
    return value.visit([](const auto& current) { return current != 0; });
}

constexpr MathValue builtin_prefix_identity(const MathValue& value)
{
    return value;
}

constexpr MathValue builtin_prefix_negate(const MathValue& value)
{
    return value.visit([](const auto& current) { return MathValue(-current); });
}

constexpr MathValue builtin_prefix_logical_not(const MathValue& value)
{
    return MathValue(builtin_truthy(value) ? 0 : 1);
}

inline MathValue builtin_prefix_abs(const MathValue& value)
{
    return value.visit([](const auto& current) -> MathValue {
        using value_t = std::decay_t<decltype(current)>;
        if constexpr (std::is_unsigned_v<value_t>)
        {
            return MathValue(current);
        }
        else
        {
            return MathValue(std::abs(current));
        }
    });
}

inline MathValue builtin_prefix_sqrt(const MathValue& value)
{
    return MathValue(std::sqrt(value.to_double()));
}

constexpr MathValue builtin_infix_add(const MathValue& left, const MathValue& right)
{
    return left + right;
}

constexpr MathValue builtin_infix_subtract(const MathValue& left, const MathValue& right)
{
    return left - right;
}

constexpr MathValue builtin_infix_multiply(const MathValue& left, const MathValue& right)
{
    return left * right;
}

constexpr MathValue builtin_infix_divide(const MathValue& left, const MathValue& right)
{
    return left / right;
}

constexpr MathValue builtin_infix_modulo(const MathValue& left, const MathValue& right)
{
    return left % right;
}

inline MathValue builtin_infix_power(const MathValue& left, const MathValue& right)
{
    return MathValue(std::pow(left.to_double(), right.to_double()));
}

inline MathValue builtin_infix_log(const MathValue& left, const MathValue& right)
{
    return MathValue(std::log(right.to_double()) / std::log(left.to_double()));
}

constexpr MathValue builtin_infix_max(const MathValue& left, const MathValue& right)
{
    return left > right ? left : right;
}

constexpr MathValue builtin_infix_min(const MathValue& left, const MathValue& right)
{
    return left < right ? left : right;
}

constexpr MathValue builtin_infix_equal(const MathValue& left, const MathValue& right)
{
    return MathValue(left == right ? 1 : 0);
}

constexpr MathValue builtin_infix_not_equal(const MathValue& left, const MathValue& right)
{
    return MathValue(left != right ? 1 : 0);
}

constexpr MathValue builtin_infix_less(const MathValue& left, const MathValue& right)
{
    return MathValue(left < right ? 1 : 0);
}

constexpr MathValue builtin_infix_less_equal(const MathValue& left, const MathValue& right)
{
    return MathValue(left <= right ? 1 : 0);
}

constexpr MathValue builtin_infix_greater(const MathValue& left, const MathValue& right)
{
    return MathValue(left > right ? 1 : 0);
}

constexpr MathValue builtin_infix_greater_equal(const MathValue& left, const MathValue& right)
{
    return MathValue(left >= right ? 1 : 0);
}

constexpr MathValue builtin_infix_logical_and(const MathValue& left, const MathValue& right)
{
    return MathValue(builtin_truthy(left) && builtin_truthy(right) ? 1 : 0);
}

constexpr MathValue builtin_infix_logical_or(const MathValue& left, const MathValue& right)
{
    return MathValue(builtin_truthy(left) || builtin_truthy(right) ? 1 : 0);
}

inline MathValue builtin_postfix_factorial(const MathValue& value)
{
    return value.visit([](const auto& current) -> MathValue {
        using value_t = std::decay_t<decltype(current)>;
        if constexpr (std::is_floating_point_v<value_t>)
        {
            return MathValue(detail::factorial_value(current));
        }
        else
        {
            return MathValue(static_cast<value_t>(detail::factorial_value(static_cast<long double>(current))));
        }
    });
}

inline MathValue builtin_function_sin(const MathValue* arguments, std::size_t)
{
    return MathValue(std::sin(arguments[0].to_double()));
}

inline MathValue builtin_function_cos(const MathValue* arguments, std::size_t)
{
    return MathValue(std::cos(arguments[0].to_double()));
}

inline MathValue builtin_function_tan(const MathValue* arguments, std::size_t)
{
    return MathValue(std::tan(arguments[0].to_double()));
}

inline MathValue builtin_function_asin(const MathValue* arguments, std::size_t)
{
    return MathValue(std::asin(arguments[0].to_double()));
}

inline MathValue builtin_function_acos(const MathValue* arguments, std::size_t)
{
    return MathValue(std::acos(arguments[0].to_double()));
}

inline MathValue builtin_function_atan(const MathValue* arguments, std::size_t)
{
    return MathValue(std::atan(arguments[0].to_double()));
}

inline MathValue builtin_function_sqrt(const MathValue* arguments, std::size_t)
{
    return MathValue(std::sqrt(arguments[0].to_double()));
}

inline MathValue builtin_function_ceil(const MathValue* arguments, std::size_t)
{
    return MathValue(std::ceil(arguments[0].to_double()));
}

inline MathValue builtin_function_floor(const MathValue* arguments, std::size_t)
{
    return MathValue(std::floor(arguments[0].to_double()));
}

inline MathValue builtin_function_round(const MathValue* arguments, std::size_t)
{
    return MathValue(std::round(arguments[0].to_double()));
}

inline MathValue builtin_function_exp(const MathValue* arguments, std::size_t)
{
    return MathValue(std::exp(arguments[0].to_double()));
}

inline MathValue builtin_function_abs(const MathValue* arguments, std::size_t)
{
    return builtin_prefix_abs(arguments[0]);
}

inline MathValue builtin_function_rad(const MathValue* arguments, std::size_t)
{
    return MathValue(arguments[0].to_double() * (3.14159265358979323846 / 180.0));
}

inline MathValue builtin_function_deg(const MathValue* arguments, std::size_t)
{
    return MathValue(arguments[0].to_double() * (180.0 / 3.14159265358979323846));
}

constexpr MathValue builtin_function_max(const MathValue* arguments, std::size_t count)
{
    MathValue current = arguments[0];
    for (std::size_t index = 1; index < count; ++index)
    {
        if (arguments[index] > current)
        {
            current = arguments[index];
        }
    }
    return current;
}

constexpr MathValue builtin_function_min(const MathValue* arguments, std::size_t count)
{
    MathValue current = arguments[0];
    for (std::size_t index = 1; index < count; ++index)
    {
        if (arguments[index] < current)
        {
            current = arguments[index];
        }
    }
    return current;
}

} // namespace string_math::detail
