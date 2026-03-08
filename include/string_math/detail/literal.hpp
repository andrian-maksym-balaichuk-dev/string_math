#pragma once

#include <string_math/value.hpp>

namespace string_math
{

inline MathValue parse_builtin_literal(std::string_view token)
{
    const std::string_view trimmed = detail::trim(token);
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
        return MathValue(detail::parse_floating_runtime<float>(text.substr(0, text.size() - 1)));
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
            return MathValue(detail::parse_floating_runtime<long double>(text));
        }
        return MathValue(detail::parse_floating_runtime<double>(text));
    }

    int base = 10;
    if (detail::starts_with(text, "0x") || detail::starts_with(text, "0X"))
    {
        base = 16;
        text.remove_prefix(2);
    }
    else if (detail::starts_with(text, "0b") || detail::starts_with(text, "0B"))
    {
        base = 2;
        text.remove_prefix(2);
    }
    else if (detail::starts_with(text, "0o") || detail::starts_with(text, "0O"))
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
        case fixed_integral_suffix::I8: return MathValue(detail::parse_integral_runtime<std::int8_t>(text, base));
        case fixed_integral_suffix::U8: return MathValue(detail::parse_integral_runtime<std::uint8_t>(text, base));
        case fixed_integral_suffix::I16: return MathValue(detail::parse_integral_runtime<std::int16_t>(text, base));
        case fixed_integral_suffix::U16: return MathValue(detail::parse_integral_runtime<std::uint16_t>(text, base));
        case fixed_integral_suffix::I32: return MathValue(detail::parse_integral_runtime<std::int32_t>(text, base));
        case fixed_integral_suffix::U32: return MathValue(detail::parse_integral_runtime<std::uint32_t>(text, base));
        case fixed_integral_suffix::I64: return MathValue(detail::parse_integral_runtime<std::int64_t>(text, base));
        case fixed_integral_suffix::U64: return MathValue(detail::parse_integral_runtime<std::uint64_t>(text, base));
        case fixed_integral_suffix::Z:
            return MathValue(detail::parse_integral_runtime<std::make_signed_t<std::size_t>>(text, base));
        case fixed_integral_suffix::UZ: return MathValue(detail::parse_integral_runtime<std::size_t>(text, base));
        case fixed_integral_suffix::None: break;
        }
    }

    if (unsigned_suffix)
    {
        if (long_suffix_count == 2)
        {
            return MathValue(detail::parse_integral_runtime<unsigned long long>(text, base));
        }
        if (long_suffix_count == 1)
        {
            return MathValue(detail::parse_integral_runtime<unsigned long>(text, base));
        }

        const unsigned long long value = detail::parse_integral_runtime<unsigned long long>(text, base);
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
        return MathValue(detail::parse_integral_runtime<long long>(text, base));
    }
    if (long_suffix_count == 1)
    {
        return MathValue(detail::parse_integral_runtime<long>(text, base));
    }

    const unsigned long long value = detail::parse_integral_runtime<unsigned long long>(text, base);
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

} // namespace string_math
