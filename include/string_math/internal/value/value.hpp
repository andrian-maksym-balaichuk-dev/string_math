#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <string_math/internal/support/config.hpp>
#include <string_math/internal/error/error.hpp>

// --- type_meta ---------------------------------------------------------------
namespace string_math::internal
{

template <class...>
inline constexpr bool always_false_v = false;

template <class Sig>
struct signature_traits;

template <class R, class... Args>
struct signature_traits<R(Args...)>
{
    using result_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <class R, class... Args>
struct signature_traits<R (*)(Args...)> : signature_traits<R(Args...)>
{};

template <class T>
struct remove_cvref
{
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template <class T>
using remove_cvref_t = typename remove_cvref<T>::type;

template <class T>
struct is_character_like : std::false_type
{};

template <>
struct is_character_like<char> : std::true_type
{};

template <>
struct is_character_like<wchar_t> : std::true_type
{};

template <>
struct is_character_like<char16_t> : std::true_type
{};

template <>
struct is_character_like<char32_t> : std::true_type
{};

#if STRING_MATH_HAS_CXX20
template <>
struct is_character_like<char8_t> : std::true_type
{};
#endif

template <class T>
inline constexpr bool is_character_like_v = is_character_like<remove_cvref_t<T>>::value;

template <class T>
inline constexpr bool is_supported_integral_input_v =
    std::is_integral_v<remove_cvref_t<T>> && !std::is_same_v<remove_cvref_t<T>, bool> && !is_character_like_v<T>;

template <class T>
struct type_identity
{
    using type = T;
};

} // namespace string_math::internal

// --- types -------------------------------------------------------------------
namespace string_math
{

enum class ValueType
{
    Short,
    UnsignedShort,
    Int,
    UnsignedInt,
    Long,
    UnsignedLong,
    LongLong,
    UnsignedLongLong,
    Float,
    Double,
    LongDouble
};

enum class Associativity
{
    Left,
    Right
};

enum class OverflowPolicy
{
    Wrap,
    Checked,
    Saturate,
    Promote
};

enum class PromotionPolicy
{
    CppLike,
    PreserveType,
    WidenToFloating
};

enum class NarrowingPolicy
{
    Allow,
    Checked
};

struct EvaluationPolicy
{
    OverflowPolicy overflow{OverflowPolicy::Wrap};
    PromotionPolicy promotion{PromotionPolicy::CppLike};
    NarrowingPolicy narrowing{NarrowingPolicy::Allow};
};

} // namespace string_math

namespace string_math::internal
{

template <class T, class Enable = void>
struct canonical_storage_type
{
    using type = void;
};

template <class T>
struct canonical_storage_selector
{
private:
    using raw_t = remove_cvref_t<T>;

    static constexpr auto select()
    {
        if constexpr (std::is_same_v<raw_t, float>)
        {
            return type_identity<float>{};
        }
        else if constexpr (std::is_same_v<raw_t, double>)
        {
            return type_identity<double>{};
        }
        else if constexpr (std::is_same_v<raw_t, long double>)
        {
            return type_identity<long double>{};
        }
        else if constexpr (std::is_same_v<raw_t, std::int8_t> || std::is_same_v<raw_t, signed char>)
        {
            return type_identity<short>{};
        }
        else if constexpr (std::is_same_v<raw_t, std::uint8_t> || std::is_same_v<raw_t, unsigned char>)
        {
            return type_identity<unsigned short>{};
        }
        else if constexpr (std::is_same_v<raw_t, short>)
        {
            return type_identity<short>{};
        }
        else if constexpr (std::is_same_v<raw_t, unsigned short>)
        {
            return type_identity<unsigned short>{};
        }
        else if constexpr (std::is_same_v<raw_t, int>)
        {
            return type_identity<int>{};
        }
        else if constexpr (std::is_same_v<raw_t, unsigned int>)
        {
            return type_identity<unsigned int>{};
        }
        else if constexpr (std::is_same_v<raw_t, long>)
        {
            return type_identity<long>{};
        }
        else if constexpr (std::is_same_v<raw_t, unsigned long>)
        {
            return type_identity<unsigned long>{};
        }
        else if constexpr (std::is_same_v<raw_t, long long>)
        {
            return type_identity<long long>{};
        }
        else if constexpr (std::is_same_v<raw_t, unsigned long long>)
        {
            return type_identity<unsigned long long>{};
        }
        else if constexpr (std::is_integral_v<raw_t> && std::is_signed_v<raw_t> && sizeof(raw_t) == 1)
        {
            return type_identity<short>{};
        }
        else if constexpr (std::is_integral_v<raw_t> && std::is_unsigned_v<raw_t> && sizeof(raw_t) == 1)
        {
            return type_identity<unsigned short>{};
        }
        else if constexpr (std::is_integral_v<raw_t> && std::is_signed_v<raw_t> && sizeof(raw_t) <= sizeof(short))
        {
            return type_identity<short>{};
        }
        else if constexpr (std::is_integral_v<raw_t> && std::is_unsigned_v<raw_t> && sizeof(raw_t) <= sizeof(unsigned short))
        {
            return type_identity<unsigned short>{};
        }
        else if constexpr (std::is_integral_v<raw_t> && std::is_signed_v<raw_t> && sizeof(raw_t) <= sizeof(int))
        {
            return type_identity<int>{};
        }
        else if constexpr (std::is_integral_v<raw_t> && std::is_unsigned_v<raw_t> && sizeof(raw_t) <= sizeof(unsigned int))
        {
            return type_identity<unsigned int>{};
        }
        else if constexpr (std::is_integral_v<raw_t> && std::is_signed_v<raw_t> && sizeof(raw_t) <= sizeof(long))
        {
            return type_identity<long>{};
        }
        else if constexpr (std::is_integral_v<raw_t> && std::is_unsigned_v<raw_t> && sizeof(raw_t) <= sizeof(unsigned long))
        {
            return type_identity<unsigned long>{};
        }
        else if constexpr (std::is_integral_v<raw_t> && std::is_signed_v<raw_t> && sizeof(raw_t) <= sizeof(long long))
        {
            return type_identity<long long>{};
        }
        else if constexpr (std::is_integral_v<raw_t> && std::is_unsigned_v<raw_t> &&
                           sizeof(raw_t) <= sizeof(unsigned long long))
        {
            return type_identity<unsigned long long>{};
        }
        else
        {
            return type_identity<void>{};
        }
    }

public:
    using type = typename decltype(select())::type;
};

template <class T>
struct canonical_storage_type<T,
                              std::enable_if_t<std::is_arithmetic_v<remove_cvref_t<T>> &&
                                               !std::is_same_v<remove_cvref_t<T>, bool> && !is_character_like_v<T>>>
    : canonical_storage_selector<T>
{};

template <class T>
using canonical_storage_type_t = typename canonical_storage_type<T>::type;

template <class T>
struct is_supported_value_type : std::bool_constant<!std::is_same_v<canonical_storage_type_t<T>, void>>
{};

template <class T>
inline constexpr bool is_supported_value_type_v = is_supported_value_type<T>::value;

template <class T>
struct value_type_of : value_type_of<canonical_storage_type_t<T>>
{};

template <>
struct value_type_of<short>
{
    static constexpr ValueType value = ValueType::Short;
};

template <>
struct value_type_of<unsigned short>
{
    static constexpr ValueType value = ValueType::UnsignedShort;
};

template <>
struct value_type_of<int>
{
    static constexpr ValueType value = ValueType::Int;
};

template <>
struct value_type_of<unsigned int>
{
    static constexpr ValueType value = ValueType::UnsignedInt;
};

template <>
struct value_type_of<long>
{
    static constexpr ValueType value = ValueType::Long;
};

template <>
struct value_type_of<unsigned long>
{
    static constexpr ValueType value = ValueType::UnsignedLong;
};

template <>
struct value_type_of<long long>
{
    static constexpr ValueType value = ValueType::LongLong;
};

template <>
struct value_type_of<unsigned long long>
{
    static constexpr ValueType value = ValueType::UnsignedLongLong;
};

template <>
struct value_type_of<float>
{
    static constexpr ValueType value = ValueType::Float;
};

template <>
struct value_type_of<double>
{
    static constexpr ValueType value = ValueType::Double;
};

template <>
struct value_type_of<long double>
{
    static constexpr ValueType value = ValueType::LongDouble;
};

template <ValueType Type>
struct type_of_value;

template <>
struct type_of_value<ValueType::Short>
{
    using type = short;
};

template <>
struct type_of_value<ValueType::UnsignedShort>
{
    using type = unsigned short;
};

template <>
struct type_of_value<ValueType::Int>
{
    using type = int;
};

template <>
struct type_of_value<ValueType::UnsignedInt>
{
    using type = unsigned int;
};

template <>
struct type_of_value<ValueType::Long>
{
    using type = long;
};

template <>
struct type_of_value<ValueType::UnsignedLong>
{
    using type = unsigned long;
};

template <>
struct type_of_value<ValueType::LongLong>
{
    using type = long long;
};

template <>
struct type_of_value<ValueType::UnsignedLongLong>
{
    using type = unsigned long long;
};

template <>
struct type_of_value<ValueType::Float>
{
    using type = float;
};

template <>
struct type_of_value<ValueType::Double>
{
    using type = double;
};

template <>
struct type_of_value<ValueType::LongDouble>
{
    using type = long double;
};

template <ValueType Type>
using type_of_value_t = typename type_of_value<Type>::type;

constexpr bool is_integral(ValueType type) noexcept
{
    return type == ValueType::Short || type == ValueType::UnsignedShort || type == ValueType::Int ||
        type == ValueType::UnsignedInt || type == ValueType::Long || type == ValueType::UnsignedLong ||
        type == ValueType::LongLong || type == ValueType::UnsignedLongLong;
}

constexpr bool is_floating(ValueType type) noexcept
{
    return type == ValueType::Float || type == ValueType::Double || type == ValueType::LongDouble;
}

constexpr int type_rank(ValueType type) noexcept
{
    switch (type)
    {
    case ValueType::Short: return 1;
    case ValueType::UnsignedShort: return 2;
    case ValueType::Int: return 3;
    case ValueType::UnsignedInt: return 4;
    case ValueType::Long: return 5;
    case ValueType::UnsignedLong: return 6;
    case ValueType::LongLong: return 7;
    case ValueType::UnsignedLongLong: return 8;
    case ValueType::Float: return 9;
    case ValueType::Double: return 10;
    case ValueType::LongDouble: return 11;
    }

    return 0;
}

constexpr bool can_convert(ValueType from, ValueType to) noexcept
{
    if (from == to)
    {
        return true;
    }

    if (is_floating(to))
    {
        return true;
    }

    if (is_integral(from) && is_integral(to))
    {
        return true;
    }

    return false;
}

constexpr int conversion_cost(ValueType from, ValueType to) noexcept
{
    if (!can_convert(from, to))
    {
        return 1'000'000;
    }

    if (from == to)
    {
        return 0;
    }

    if (is_integral(from) && is_integral(to))
    {
        return 100 + std::max(0, type_rank(to) - type_rank(from));
    }

    if (is_integral(from) && is_floating(to))
    {
        return 200 + type_rank(to);
    }

    if (is_floating(from) && is_floating(to))
    {
        return 300 + std::max(0, type_rank(to) - type_rank(from));
    }

    return 900 + type_rank(to);
}

template <class Left, class Right>
using binary_promotion_type_t = remove_cvref_t<decltype(std::declval<Left>() + std::declval<Right>())>;

template <class Left, class Right>
struct binary_promotion_value_type
{
    using promoted_type = canonical_storage_type_t<binary_promotion_type_t<Left, Right>>;
    static_assert(is_supported_value_type_v<promoted_type>, "unsupported promoted value type");
    static constexpr ValueType value = value_type_of<promoted_type>::value;
};

template <ValueType LeftType, ValueType RightType>
constexpr ValueType preferred_binary_target_for() noexcept
{
    return binary_promotion_value_type<type_of_value_t<LeftType>, type_of_value_t<RightType>>::value;
}

constexpr std::size_t value_type_count = static_cast<std::size_t>(ValueType::LongDouble) + 1;

constexpr std::size_t value_type_index(ValueType value) noexcept
{
    return static_cast<std::size_t>(value);
}

template <std::size_t... Indices>
constexpr auto make_preferred_binary_target_table(std::index_sequence<Indices...>)
{
    return std::array<ValueType, sizeof...(Indices)>{
        preferred_binary_target_for<static_cast<ValueType>(Indices / value_type_count), static_cast<ValueType>(Indices % value_type_count)>()...,
    };
}

constexpr ValueType preferred_binary_target(ValueType left, ValueType right) noexcept
{
    constexpr auto table = make_preferred_binary_target_table(std::make_index_sequence<value_type_count * value_type_count>{});
    return table[(value_type_index(left) * value_type_count) + value_type_index(right)];
}

} // namespace string_math::internal

namespace string_math
{

template <class T, class Enable = void>
struct ValueTraits
{
    static constexpr bool supported = false;
};

template <class T>
struct ValueTraits<T, std::enable_if_t<internal::is_supported_value_type_v<T>>>
{
    static constexpr bool supported = true;
    using value_type = internal::canonical_storage_type_t<T>;

    static constexpr value_type to_storage(T value)
    {
        return static_cast<value_type>(value);
    }

    static constexpr T from_storage(value_type value)
    {
        return static_cast<T>(value);
    }
};

template <class T>
inline constexpr bool has_value_traits_v = ValueTraits<internal::remove_cvref_t<T>>::supported;

} // namespace string_math

// --- value -------------------------------------------------------------------
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
