#pragma once

#include <cmath>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <variant>

#include <string_math/internal/config.hpp>
#include <string_math/internal/type_conversion.hpp>
#include <string_math/value/policy.hpp>
#include <string_math/diagnostics/result.hpp>
#include <string_math/value/traits.hpp>

namespace string_math
{

class MathValue
{
public:
    using storage_type = std::variant<STRING_MATH_ALL_VALUE_TYPES>;

    constexpr MathValue() : m_value(0) {}

    template <class T, std::enable_if_t<has_value_traits_v<T>, int> = 0>
    constexpr MathValue(T value) : m_value(ValueTraits<internal::remove_cvref_t<T>>::to_storage(value))
    {}

    constexpr ValueType type() const noexcept
    {
        return static_cast<ValueType>(m_value.index());
    }

    template <class T>
    constexpr bool is() const noexcept
    {
        static_assert(has_value_traits_v<T> || internal::is_supported_value_type_v<T>, "unsupported value type");
        using storage_t = std::conditional_t<
            has_value_traits_v<T>,
            typename ValueTraits<internal::remove_cvref_t<T>>::value_type,
            internal::canonical_storage_type_t<T>>;
        return std::holds_alternative<storage_t>(m_value);
    }

    template <class T>
    constexpr T get() const
    {
        static_assert(internal::is_supported_value_type_v<T>, "unsupported value type");
        return cast<T>();
    }

    template <class T>
    constexpr T cast() const
    {
        static_assert(has_value_traits_v<T> || internal::is_supported_value_type_v<T>, "unsupported value type");
        return std::visit(
            [](const auto& value) -> T {
                if constexpr (has_value_traits_v<T>)
                {
                    using storage_t = typename ValueTraits<internal::remove_cvref_t<T>>::value_type;
                    return ValueTraits<internal::remove_cvref_t<T>>::from_storage(static_cast<storage_t>(value));
                }
                else
                {
                    return static_cast<T>(value);
                }
            },
            m_value);
    }

    constexpr double to_double() const
    {
        return cast<double>();
    }

    std::string to_string() const
    {
        return std::visit(
            [](const auto& value) {
                using value_t = std::decay_t<decltype(value)>;
                std::ostringstream stream;
                if constexpr (std::is_same_v<value_t, float>)
                {
                    stream << value << 'f';
                }
                else if constexpr (std::is_same_v<value_t, unsigned int>)
                {
                    stream << value << 'u';
                }
                else if constexpr (std::is_same_v<value_t, long>)
                {
                    stream << value << 'l';
                }
                else if constexpr (std::is_same_v<value_t, unsigned long>)
                {
                    stream << value << "ul";
                }
                else if constexpr (std::is_same_v<value_t, long long>)
                {
                    stream << value << "ll";
                }
                else if constexpr (std::is_same_v<value_t, unsigned long long>)
                {
                    stream << value << "ull";
                }
                else if constexpr (std::is_same_v<value_t, long double>)
                {
                    stream << value << 'L';
                }
                else
                {
                    stream << value;
                }
                return stream.str();
            },
            m_value);
    }

    template <class Visitor>
    constexpr decltype(auto) visit(Visitor&& visitor) const
    {
        return std::visit(std::forward<Visitor>(visitor), m_value);
    }

private:
    storage_type m_value;
};

inline std::string to_string(const MathValue& value)
{
    return value.to_string();
}

template <class T>
inline T cast_value(const MathValue& value, NarrowingPolicy policy = NarrowingPolicy::Allow)
{
    const T converted = value.template cast<T>();
    if (policy == NarrowingPolicy::Allow)
    {
        return converted;
    }

    if constexpr (std::is_integral_v<T>)
    {
        const long double as_long_double = value.visit([](const auto& current) { return static_cast<long double>(current); });
        if (as_long_double < static_cast<long double>(std::numeric_limits<T>::lowest()) ||
            as_long_double > static_cast<long double>(std::numeric_limits<T>::max()))
        {
            throw std::out_of_range("string_math: narrowing conversion out of range");
        }
        if (static_cast<long double>(converted) != as_long_double)
        {
            throw std::out_of_range("string_math: narrowing conversion loses precision");
        }
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        const long double as_long_double = value.visit([](const auto& current) { return static_cast<long double>(current); });
        if (!std::isfinite(static_cast<double>(converted)) && std::isfinite(static_cast<double>(as_long_double)))
        {
            throw std::out_of_range("string_math: narrowing conversion out of range");
        }
    }

    return converted;
}

namespace internal
{

template <class T>
constexpr bool add_overflow(T left, T right, T& output)
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
constexpr bool sub_overflow(T left, T right, T& output)
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
inline bool mul_overflow(T left, T right, T& output)
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
constexpr T saturate_cast(long double value)
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

template <class Operator>
constexpr MathValue apply_binary_value_operation(const MathValue& left, const MathValue& right, Operator op)
{
    const ValueType target = preferred_binary_target(left.type(), right.type());
    switch (target)
    {
    case ValueType::Short: return MathValue(op(left.cast<short>(), right.cast<short>()));
    case ValueType::UnsignedShort: return MathValue(op(left.cast<unsigned short>(), right.cast<unsigned short>()));
    case ValueType::Int: return MathValue(op(left.cast<int>(), right.cast<int>()));
    case ValueType::UnsignedInt: return MathValue(op(left.cast<unsigned int>(), right.cast<unsigned int>()));
    case ValueType::Long: return MathValue(op(left.cast<long>(), right.cast<long>()));
    case ValueType::UnsignedLong: return MathValue(op(left.cast<unsigned long>(), right.cast<unsigned long>()));
    case ValueType::LongLong: return MathValue(op(left.cast<long long>(), right.cast<long long>()));
    case ValueType::UnsignedLongLong:
        return MathValue(op(left.cast<unsigned long long>(), right.cast<unsigned long long>()));
    case ValueType::Float: return MathValue(op(left.cast<float>(), right.cast<float>()));
    case ValueType::Double: return MathValue(op(left.cast<double>(), right.cast<double>()));
    case ValueType::LongDouble: return MathValue(op(left.cast<long double>(), right.cast<long double>()));
    }

    throw std::logic_error("string_math: unsupported math value type");
}

template <class Compare>
constexpr bool apply_compare_value_operation(const MathValue& left, const MathValue& right, Compare compare)
{
    return left.visit([&](const auto& lhs) {
        return right.visit([&](const auto& rhs) {
            using compare_t = std::common_type_t<decltype(lhs), decltype(rhs)>;
            return compare(static_cast<compare_t>(lhs), static_cast<compare_t>(rhs));
        });
    });
}

} // namespace internal

constexpr bool operator==(const MathValue& left, const MathValue& right)
{
    return internal::apply_compare_value_operation(left, right, [](const auto& lhs, const auto& rhs) { return lhs == rhs; });
}

constexpr bool operator!=(const MathValue& left, const MathValue& right)
{
    return !(left == right);
}

constexpr bool operator<(const MathValue& left, const MathValue& right)
{
    return internal::apply_compare_value_operation(left, right, [](const auto& lhs, const auto& rhs) { return lhs < rhs; });
}

constexpr bool operator<=(const MathValue& left, const MathValue& right)
{
    return internal::apply_compare_value_operation(left, right, [](const auto& lhs, const auto& rhs) { return lhs <= rhs; });
}

constexpr bool operator>(const MathValue& left, const MathValue& right)
{
    return internal::apply_compare_value_operation(left, right, [](const auto& lhs, const auto& rhs) { return lhs > rhs; });
}

constexpr bool operator>=(const MathValue& left, const MathValue& right)
{
    return internal::apply_compare_value_operation(left, right, [](const auto& lhs, const auto& rhs) { return lhs >= rhs; });
}

template <class T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
constexpr bool operator==(const MathValue& left, T right)
{
    return left.visit([&](const auto& lhs) {
        using compare_t = std::common_type_t<decltype(lhs), T>;
        return static_cast<compare_t>(lhs) == static_cast<compare_t>(right);
    });
}

template <class T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
constexpr bool operator==(T left, const MathValue& right)
{
    return right == left;
}

template <class T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
constexpr bool operator!=(const MathValue& left, T right)
{
    return !(left == right);
}

template <class T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
constexpr bool operator!=(T left, const MathValue& right)
{
    return !(left == right);
}

constexpr MathValue operator+(const MathValue& left, const MathValue& right)
{
    return internal::apply_binary_value_operation(left, right, [](const auto& lhs, const auto& rhs) { return lhs + rhs; });
}

constexpr MathValue operator+(const MathValue& value)
{
    return value;
}

constexpr MathValue operator-(const MathValue& left, const MathValue& right)
{
    return internal::apply_binary_value_operation(left, right, [](const auto& lhs, const auto& rhs) { return lhs - rhs; });
}

constexpr MathValue operator-(const MathValue& value)
{
    return value.visit([](const auto& current) { return MathValue(-current); });
}

constexpr MathValue operator*(const MathValue& left, const MathValue& right)
{
    return internal::apply_binary_value_operation(left, right, [](const auto& lhs, const auto& rhs) { return lhs * rhs; });
}

constexpr MathValue operator/(const MathValue& left, const MathValue& right)
{
    return internal::apply_binary_value_operation(left, right, [](const auto& lhs, const auto& rhs) {
        if (rhs == 0)
        {
            throw std::domain_error("string_math: division by zero");
        }
        return lhs / rhs;
    });
}

constexpr MathValue operator%(const MathValue& left, const MathValue& right)
{
    const ValueType target = internal::preferred_binary_target(left.type(), right.type());
    if (internal::is_integral(target))
    {
        return internal::apply_binary_value_operation(left, right, [](const auto& lhs, const auto& rhs) {
            if (rhs == 0)
            {
                throw std::domain_error("string_math: modulo by zero");
            }
            using value_t = std::decay_t<decltype(lhs)>;
            if constexpr (std::is_integral_v<value_t>)
            {
                return lhs % rhs;
            }
            else
            {
                return std::fmod(lhs, rhs);
            }
        });
    }

    switch (target)
    {
    case ValueType::Float: return MathValue(std::fmod(left.cast<float>(), right.cast<float>()));
    case ValueType::Double: return MathValue(std::fmod(left.cast<double>(), right.cast<double>()));
    case ValueType::LongDouble: return MathValue(std::fmod(left.cast<long double>(), right.cast<long double>()));
    default: break;
    }

    throw std::logic_error("string_math: unsupported modulo target");
}

inline std::ostream& operator<<(std::ostream& stream, const MathValue& value)
{
    stream << value.to_string();
    return stream;
}

template <class T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
constexpr MathValue operator+(const MathValue& left, T right)
{
    return left + MathValue(right);
}

template <class T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
constexpr MathValue operator+(T left, const MathValue& right)
{
    return MathValue(left) + right;
}

template <class T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
constexpr MathValue operator-(const MathValue& left, T right)
{
    return left - MathValue(right);
}

template <class T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
constexpr MathValue operator-(T left, const MathValue& right)
{
    return MathValue(left) - right;
}

template <class T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
constexpr MathValue operator*(const MathValue& left, T right)
{
    return left * MathValue(right);
}

template <class T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
constexpr MathValue operator*(T left, const MathValue& right)
{
    return MathValue(left) * right;
}

template <class T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
constexpr MathValue operator/(const MathValue& left, T right)
{
    return left / MathValue(right);
}

template <class T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
constexpr MathValue operator/(T left, const MathValue& right)
{
    return MathValue(left) / right;
}

template <class T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
constexpr MathValue operator%(const MathValue& left, T right)
{
    return left % MathValue(right);
}

template <class T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
constexpr MathValue operator%(T left, const MathValue& right)
{
    return MathValue(left) % right;
}

} // namespace string_math
