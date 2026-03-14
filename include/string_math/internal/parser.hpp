#pragma once
// Do not include this file directly. Use <string_math/string_math.hpp> or individual public headers.

#include <cstdint>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <string_math/internal/config.hpp>
#include <string_math/value/value.hpp>
#include <string_math/diagnostics/error.hpp>

namespace string_math::internal
{

// ---------------------------------------------------------------------------
// String / character utilities
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Builtin literal parser
// ---------------------------------------------------------------------------

inline MathValue parse_builtin_literal(std::string_view token)
{
    const std::string_view trimmed = trim(token);
    if (trimmed.empty())
    {
        throw std::invalid_argument("string_math: empty literal");
    }

    std::string_view text = trimmed;
    bool unsigned_suffix = false;
    int long_suffix_count = 0;
    enum class fixed_integral_suffix
    {
        None,
        I8,
        U8,
        I16,
        U16,
        I32,
        U32,
        I64,
        U64,
        Z,
        UZ
    };
    fixed_integral_suffix fixed_suffix = fixed_integral_suffix::None;

    const auto matches_suffix = [](std::string_view value, std::string_view suffix) {
        if (value.size() < suffix.size())
        {
            return false;
        }

        const std::string_view tail = value.substr(value.size() - suffix.size());
        for (std::size_t index = 0; index < suffix.size(); ++index)
        {
            const char expected = suffix[index];
            const char current = tail[index];
            if (expected >= 'a' && expected <= 'z')
            {
                if (current != expected && current != static_cast<char>(expected - ('a' - 'A')))
                {
                    return false;
                }
            }
            else if (current != expected)
            {
                return false;
            }
        }

        return true;
    };

    const auto consume_fixed_suffix = [&](std::string_view suffix, fixed_integral_suffix kind) {
        if (matches_suffix(text, suffix))
        {
            fixed_suffix = kind;
            text.remove_suffix(suffix.size());
            return true;
        }
        return false;
    };

    if (!text.empty() && (text.back() == 'f' || text.back() == 'F'))
    {
        return MathValue(parse_floating_runtime<float>(text.substr(0, text.size() - 1)));
    }

    consume_fixed_suffix("uz", fixed_integral_suffix::UZ) || consume_fixed_suffix("u64", fixed_integral_suffix::U64) ||
        consume_fixed_suffix("i64", fixed_integral_suffix::I64) || consume_fixed_suffix("u32", fixed_integral_suffix::U32) ||
        consume_fixed_suffix("i32", fixed_integral_suffix::I32) || consume_fixed_suffix("u16", fixed_integral_suffix::U16) ||
        consume_fixed_suffix("i16", fixed_integral_suffix::I16) || consume_fixed_suffix("u8", fixed_integral_suffix::U8) ||
        consume_fixed_suffix("i8", fixed_integral_suffix::I8) || consume_fixed_suffix("z", fixed_integral_suffix::Z);

    const bool is_floating_literal = text.find('.') != std::string_view::npos ||
        text.find('e') != std::string_view::npos || text.find('E') != std::string_view::npos;

    bool changed = true;
    while (changed && !text.empty())
    {
        changed = false;

        if (!unsigned_suffix && (text.back() == 'u' || text.back() == 'U'))
        {
            unsigned_suffix = true;
            text.remove_suffix(1);
            changed = true;
            continue;
        }

        if (long_suffix_count == 0 && text.size() >= 2)
        {
            const char last = text.back();
            const char before_last = text[text.size() - 2];
            if ((before_last == 'l' || before_last == 'L') && (last == 'l' || last == 'L'))
            {
                long_suffix_count = 2;
                text.remove_suffix(2);
                changed = true;
                continue;
            }
        }

        if (long_suffix_count == 0 && (text.back() == 'l' || text.back() == 'L'))
        {
            long_suffix_count = 1;
            text.remove_suffix(1);
            changed = true;
        }
    }

    if (text.empty())
    {
        throw std::invalid_argument("string_math: invalid literal suffix");
    }

    if (is_floating_literal)
    {
        if (fixed_suffix != fixed_integral_suffix::None || unsigned_suffix || long_suffix_count == 2)
        {
            throw std::invalid_argument("string_math: invalid floating literal suffix");
        }
        if (long_suffix_count == 1)
        {
            return MathValue(parse_floating_runtime<long double>(text));
        }
        return MathValue(parse_floating_runtime<double>(text));
    }

    int base = 10;
    if (starts_with(text, "0x") || starts_with(text, "0X"))
    {
        base = 16;
        text.remove_prefix(2);
    }
    else if (starts_with(text, "0b") || starts_with(text, "0B"))
    {
        base = 2;
        text.remove_prefix(2);
    }
    else if (starts_with(text, "0o") || starts_with(text, "0O"))
    {
        base = 8;
        text.remove_prefix(2);
    }

    if (text.empty())
    {
        throw std::invalid_argument("string_math: invalid literal");
    }

    if (fixed_suffix != fixed_integral_suffix::None)
    {
        switch (fixed_suffix)
        {
        case fixed_integral_suffix::I8: return MathValue(parse_integral_runtime<std::int8_t>(text, base));
        case fixed_integral_suffix::U8: return MathValue(parse_integral_runtime<std::uint8_t>(text, base));
        case fixed_integral_suffix::I16: return MathValue(parse_integral_runtime<std::int16_t>(text, base));
        case fixed_integral_suffix::U16: return MathValue(parse_integral_runtime<std::uint16_t>(text, base));
        case fixed_integral_suffix::I32: return MathValue(parse_integral_runtime<std::int32_t>(text, base));
        case fixed_integral_suffix::U32: return MathValue(parse_integral_runtime<std::uint32_t>(text, base));
        case fixed_integral_suffix::I64: return MathValue(parse_integral_runtime<std::int64_t>(text, base));
        case fixed_integral_suffix::U64: return MathValue(parse_integral_runtime<std::uint64_t>(text, base));
        case fixed_integral_suffix::Z:
            return MathValue(parse_integral_runtime<std::make_signed_t<std::size_t>>(text, base));
        case fixed_integral_suffix::UZ: return MathValue(parse_integral_runtime<std::size_t>(text, base));
        case fixed_integral_suffix::None: break;
        }
    }

    if (unsigned_suffix)
    {
        if (long_suffix_count == 2)
        {
            return MathValue(parse_integral_runtime<unsigned long long>(text, base));
        }
        if (long_suffix_count == 1)
        {
            return MathValue(parse_integral_runtime<unsigned long>(text, base));
        }

        const unsigned long long value = parse_integral_runtime<unsigned long long>(text, base);
        if (value <= static_cast<unsigned long long>(std::numeric_limits<unsigned int>::max()))
        {
            return MathValue(static_cast<unsigned int>(value));
        }
        if (value <= static_cast<unsigned long long>(std::numeric_limits<unsigned long>::max()))
        {
            return MathValue(static_cast<unsigned long>(value));
        }
        return MathValue(value);
    }

    if (long_suffix_count == 2)
    {
        return MathValue(parse_integral_runtime<long long>(text, base));
    }
    if (long_suffix_count == 1)
    {
        return MathValue(parse_integral_runtime<long>(text, base));
    }

    const unsigned long long value = parse_integral_runtime<unsigned long long>(text, base);
    if (base == 10)
    {
        if (value <= static_cast<unsigned long long>(std::numeric_limits<int>::max()))
        {
            return MathValue(static_cast<int>(value));
        }
        if (value <= static_cast<unsigned long long>(std::numeric_limits<long>::max()))
        {
            return MathValue(static_cast<long>(value));
        }
        if (value <= static_cast<unsigned long long>(std::numeric_limits<long long>::max()))
        {
            return MathValue(static_cast<long long>(value));
        }
        return MathValue(value);
    }

    if (value <= static_cast<unsigned long long>(std::numeric_limits<int>::max()))
    {
        return MathValue(static_cast<int>(value));
    }
    if (value <= static_cast<unsigned long long>(std::numeric_limits<unsigned int>::max()))
    {
        return MathValue(static_cast<unsigned int>(value));
    }
    if (value <= static_cast<unsigned long long>(std::numeric_limits<long>::max()))
    {
        return MathValue(static_cast<long>(value));
    }
    if (value <= static_cast<unsigned long long>(std::numeric_limits<unsigned long>::max()))
    {
        return MathValue(static_cast<unsigned long>(value));
    }
    if (value <= static_cast<unsigned long long>(std::numeric_limits<long long>::max()))
    {
        return MathValue(static_cast<long long>(value));
    }
    return MathValue(value);
}

// ---------------------------------------------------------------------------
// AST node types (Parser class is in expression/operation.hpp)
// ---------------------------------------------------------------------------

struct Node
{
    enum class Kind
    {
        Literal,
        Variable,
        PrefixOperator,
        PostfixOperator,
        InfixOperator,
        FunctionCall,
        Conditional
    };

    Kind kind{Kind::Literal};
    MathValue literal{};
    std::string text;
    int left{-1};
    int right{-1};
    int third{-1};
    int arity{0};
    std::vector<int> arguments;
    SourceSpan span{};
};

struct ParsedText
{
    std::string text;
    SourceSpan span{};
};

} // namespace string_math::internal
