#pragma once

#include <string_math/internal/config.hpp>
#include <string_math/internal/type_traits.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

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
