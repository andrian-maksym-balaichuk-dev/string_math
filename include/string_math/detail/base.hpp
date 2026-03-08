#pragma once

#include <string_math/detail/config.hpp>
#include <string_math/policy.hpp>

#include <fw/function_wrapper.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#define STRING_MATH_ALL_VALUE_TYPES \
    short, unsigned short, int, unsigned int, long, unsigned long, long long, unsigned long long, float, double, long double

#define STRING_MATH_SIGNED_AND_FLOATING_VALUE_TYPES short, int, long, long long, float, double, long double

#define STRING_MATH_UNSIGNED_INTEGRAL_VALUE_TYPES unsigned short, unsigned int, unsigned long, unsigned long long

#define STRING_MATH_SIGNED_FACTORIAL_SIGNATURES short(short), int(int), long(long), long long(long long)

#define STRING_MATH_UNSIGNED_FACTORIAL_SIGNATURES \
    unsigned short(unsigned short), unsigned int(unsigned int), unsigned long(unsigned long), unsigned long long(unsigned long long)

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

struct Precedence
{
    using value_type = int;

    static constexpr value_type Conditional = 1;
    static constexpr value_type LogicalOr = 2;
    static constexpr value_type LogicalAnd = 3;
    static constexpr value_type Comparison = 5;
    static constexpr value_type Addition = 10;
    static constexpr value_type Additive = Addition;
    static constexpr value_type Multiplication = 20;
    static constexpr value_type Multiplicative = Multiplication;
    static constexpr value_type Exponentiation = 30;
    static constexpr value_type Power = Exponentiation;
    static constexpr value_type UnaryPrefix = 40;
    static constexpr value_type Prefix = UnaryPrefix;
    static constexpr value_type UnaryPostfix = 50;
    static constexpr value_type Postfix = UnaryPostfix;
};

namespace detail
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

constexpr ValueType preferred_binary_target(ValueType left, ValueType right, PromotionPolicy policy) noexcept
{
    switch (policy)
    {
    case PromotionPolicy::CppLike:
        return preferred_binary_target(left, right);
    case PromotionPolicy::PreserveType:
        return left == right ? left : preferred_binary_target(left, right);
    case PromotionPolicy::WidenToFloating:
        if (is_floating(left) || is_floating(right))
        {
            if (left == ValueType::LongDouble || right == ValueType::LongDouble)
            {
                return ValueType::LongDouble;
            }
            return ValueType::Double;
        }
        return ValueType::Double;
    }

    return preferred_binary_target(left, right);
}

constexpr bool is_space(char ch) noexcept
{
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' || ch == '\v';
}

inline std::string_view trim(std::string_view text) noexcept
{
    std::size_t begin = 0;
    while (begin < text.size() && is_space(text[begin]))
    {
        ++begin;
    }

    std::size_t end = text.size();
    while (end > begin && is_space(text[end - 1]))
    {
        --end;
    }

    return text.substr(begin, end - begin);
}

constexpr bool starts_with(std::string_view text, std::string_view prefix) noexcept
{
    return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

constexpr bool ends_with(std::string_view text, std::string_view suffix) noexcept
{
    return text.size() >= suffix.size() && text.substr(text.size() - suffix.size()) == suffix;
}

constexpr bool is_identifier_start(char ch) noexcept
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

constexpr bool is_identifier_char(char ch) noexcept
{
    return is_identifier_start(ch) || (ch >= '0' && ch <= '9');
}

constexpr bool is_digit(char ch) noexcept
{
    return ch >= '0' && ch <= '9';
}

constexpr bool is_hex_digit(char ch) noexcept
{
    return is_digit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

constexpr int hex_value(char ch) noexcept
{
    if (ch >= '0' && ch <= '9')
    {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f')
    {
        return 10 + (ch - 'a');
    }
    return 10 + (ch - 'A');
}

constexpr long double powi(long double base, int exponent) noexcept
{
    long double result = 1.0L;
    if (exponent < 0)
    {
        for (int index = 0; index < -exponent; ++index)
        {
            result /= base;
        }
        return result;
    }

    for (int index = 0; index < exponent; ++index)
    {
        result *= base;
    }
    return result;
}

constexpr long double parse_decimal_long_double(std::string_view text)
{
    long double value = 0.0L;
    std::size_t index = 0;

    while (index < text.size() && is_digit(text[index]))
    {
        value = (value * 10.0L) + static_cast<long double>(text[index] - '0');
        ++index;
    }

    if (index < text.size() && text[index] == '.')
    {
        ++index;
        long double place = 0.1L;
        while (index < text.size() && is_digit(text[index]))
        {
            value += static_cast<long double>(text[index] - '0') * place;
            place *= 0.1L;
            ++index;
        }
    }

    if (index < text.size() && (text[index] == 'e' || text[index] == 'E'))
    {
        ++index;
        int sign = 1;
        if (index < text.size() && (text[index] == '+' || text[index] == '-'))
        {
            sign = text[index] == '-' ? -1 : 1;
            ++index;
        }

        int exponent = 0;
        while (index < text.size() && is_digit(text[index]))
        {
            exponent = (exponent * 10) + (text[index] - '0');
            ++index;
        }

        value *= powi(10.0L, sign * exponent);
    }

    return value;
}

template <class T>
T parse_floating_runtime(std::string_view text)
{
    std::string owned(text);
    char* end = nullptr;

    if constexpr (std::is_same_v<T, float>)
    {
        const float value = std::strtof(owned.c_str(), &end);
        if (end == nullptr || *end != '\0')
        {
            throw std::invalid_argument("string_math: invalid floating literal");
        }
        return value;
    }
    else if constexpr (std::is_same_v<T, double>)
    {
        const double value = std::strtod(owned.c_str(), &end);
        if (end == nullptr || *end != '\0')
        {
            throw std::invalid_argument("string_math: invalid floating literal");
        }
        return value;
    }
    else
    {
        const long double value = std::strtold(owned.c_str(), &end);
        if (end == nullptr || *end != '\0')
        {
            throw std::invalid_argument("string_math: invalid floating literal");
        }
        return value;
    }
}

template <class T>
T parse_integral_runtime(std::string_view digits, int base)
{
    using unsigned_parse_t = unsigned long long;
    unsigned_parse_t value = 0;
    for (const char ch : digits)
    {
        int digit = 0;
        if (base == 16)
        {
            if (!is_hex_digit(ch))
            {
                throw std::invalid_argument("string_math: invalid integral literal");
            }
            digit = hex_value(ch);
        }
        else
        {
            if (ch < '0' || ch >= static_cast<char>('0' + base))
            {
                throw std::invalid_argument("string_math: invalid integral literal");
            }
            digit = ch - '0';
        }
        value = (value * static_cast<unsigned_parse_t>(base)) + static_cast<unsigned_parse_t>(digit);
    }

    if (value > static_cast<unsigned_parse_t>(std::numeric_limits<T>::max()))
    {
        throw std::out_of_range("string_math: integral literal is out of range");
    }

    return static_cast<T>(value);
}

template <class T>
constexpr T parse_integral_constexpr(std::string_view digits, int base)
{
    unsigned long long value = 0;
    for (const char ch : digits)
    {
        int digit = 0;
        if (base == 16)
        {
            if (!is_hex_digit(ch))
            {
                throw std::invalid_argument("string_math: invalid integral literal");
            }
            digit = hex_value(ch);
        }
        else
        {
            if (ch < '0' || ch >= static_cast<char>('0' + base))
            {
                throw std::invalid_argument("string_math: invalid integral literal");
            }
            digit = ch - '0';
        }
        value = (value * static_cast<unsigned long long>(base)) + static_cast<unsigned long long>(digit);
    }

    if (value > static_cast<unsigned long long>(std::numeric_limits<T>::max()))
    {
        throw std::out_of_range("string_math: integral literal is out of range");
    }

    return static_cast<T>(value);
}

} // namespace detail

} // namespace string_math
