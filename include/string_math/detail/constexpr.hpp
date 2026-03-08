#pragma once

#include <string_math/value.hpp>

namespace string_math::detail
{

template <std::size_t N>
struct fixed_string
{
    std::array<char, N> value{};

    constexpr fixed_string(const char (&text)[N])
    {
        for (std::size_t index = 0; index < N; ++index)
        {
            value[index] = text[index];
        }
    }

    constexpr std::string_view view() const noexcept
    {
        return std::string_view(value.data(), N - 1);
    }
};

template <std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

class ConstexprParser
{
public:
    constexpr explicit ConstexprParser(std::string_view expression) : m_expression(expression) {}

    constexpr MathValue parse()
    {
        const MathValue result = m_parse_expression(0);
        m_skip_spaces();
        if (m_position != m_expression.size())
        {
            throw std::invalid_argument("string_math: unexpected trailing input");
        }
        return result;
    }

private:
    struct MatchedOperator
    {
        std::string_view symbol;
        int precedence{ 0 };
        Associativity associativity{ Associativity::Left };
    };

    constexpr void m_skip_spaces() noexcept
    {
        while (m_position < m_expression.size() && is_space(m_expression[m_position]))
        {
            ++m_position;
        }
    }

    constexpr MathValue m_parse_expression(int min_precedence)
    {
        MathValue left = m_parse_prefix_or_primary();

        for (;;)
        {
            m_skip_spaces();
            if (m_position >= m_expression.size() || m_expression[m_position] == ')')
            {
                break;
            }

            const auto match = m_match_infix();
            if (match.symbol.empty() || match.precedence < min_precedence)
            {
                break;
            }

            m_position += match.symbol.size();
            const int next_min = match.associativity == Associativity::Left ? match.precedence + 1 : match.precedence;
            MathValue right = m_parse_expression(next_min);
            left = m_apply_infix(match.symbol, left, right);
        }

        m_skip_spaces();
        if (min_precedence <= Precedence::Conditional && m_position < m_expression.size() && m_expression[m_position] == '?')
        {
            ++m_position;
            const MathValue true_value = m_parse_expression(0);
            m_skip_spaces();
            if (m_position >= m_expression.size() || m_expression[m_position] != ':')
            {
                throw std::invalid_argument("string_math: expected ':' in conditional expression");
            }
            ++m_position;
            const MathValue false_value = m_parse_expression(Precedence::Conditional);
            return m_truthy(left) ? true_value : false_value;
        }

        return left;
    }

    constexpr MatchedOperator m_match_infix() const noexcept
    {
        const std::string_view tail = m_expression.substr(m_position);
        if (starts_with(tail, "max"))
        {
            return { "max", Precedence::Power, Associativity::Left };
        }
        if (starts_with(tail, "min"))
        {
            return { "min", Precedence::Power, Associativity::Left };
        }
        if (starts_with(tail, "log"))
        {
            return { "log", Precedence::Power, Associativity::Left };
        }
        if (starts_with(tail, "||"))
        {
            return { "||", Precedence::LogicalOr, Associativity::Left };
        }
        if (starts_with(tail, "&&"))
        {
            return { "&&", Precedence::LogicalAnd, Associativity::Left };
        }
        if (starts_with(tail, "=="))
        {
            return { "==", Precedence::Comparison, Associativity::Left };
        }
        if (starts_with(tail, "!="))
        {
            return { "!=", Precedence::Comparison, Associativity::Left };
        }
        if (starts_with(tail, "<="))
        {
            return { "<=", Precedence::Comparison, Associativity::Left };
        }
        if (starts_with(tail, ">="))
        {
            return { ">=", Precedence::Comparison, Associativity::Left };
        }
        if (starts_with(tail, "<"))
        {
            return { "<", Precedence::Comparison, Associativity::Left };
        }
        if (starts_with(tail, ">"))
        {
            return { ">", Precedence::Comparison, Associativity::Left };
        }
        if (starts_with(tail, "^"))
        {
            return { "^", Precedence::Power, Associativity::Right };
        }
        if (starts_with(tail, "*"))
        {
            return { "*", Precedence::Multiplicative, Associativity::Left };
        }
        if (starts_with(tail, "/"))
        {
            return { "/", Precedence::Multiplicative, Associativity::Left };
        }
        if (starts_with(tail, "%"))
        {
            return { "%", Precedence::Multiplicative, Associativity::Left };
        }
        if (starts_with(tail, "+"))
        {
            return { "+", Precedence::Additive, Associativity::Left };
        }
        if (starts_with(tail, "-"))
        {
            return { "-", Precedence::Additive, Associativity::Left };
        }
        return {};
    }

    constexpr MathValue m_parse_prefix_or_primary()
    {
        m_skip_spaces();
        if (m_position >= m_expression.size())
        {
            throw std::invalid_argument("string_math: expected expression");
        }

        if (m_expression[m_position] == '+')
        {
            ++m_position;
            return m_parse_expression(Precedence::Prefix);
        }
        if (m_expression[m_position] == '-')
        {
            ++m_position;
            const MathValue value = m_parse_expression(Precedence::Prefix);
            return value.visit([](const auto& current) { return MathValue(-current); });
        }
        if (m_expression[m_position] == '!')
        {
            ++m_position;
            const MathValue value = m_parse_expression(Precedence::Prefix);
            return m_truthy(value) ? MathValue(0) : MathValue(1);
        }

        return m_parse_primary();
    }

    constexpr MathValue m_parse_primary()
    {
        m_skip_spaces();
        if (m_position >= m_expression.size())
        {
            throw std::invalid_argument("string_math: expected primary");
        }

        if (m_expression[m_position] == '(')
        {
            ++m_position;
            const MathValue inner = m_parse_expression(0);
            m_skip_spaces();
            if (m_position >= m_expression.size() || m_expression[m_position] != ')')
            {
                throw std::invalid_argument("string_math: expected ')'");
            }
            ++m_position;
            return inner;
        }

        return m_parse_constexpr_literal(m_parse_literal());
    }

    constexpr std::string_view m_parse_literal()
    {
        const std::size_t begin = m_position;
        while (m_position < m_expression.size())
        {
            const char ch = m_expression[m_position];
            if (is_space(ch) || ch == ')' || ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%' || ch == '^')
            {
                break;
            }
            ++m_position;
        }
        return m_expression.substr(begin, m_position - begin);
    }

    constexpr MathValue m_parse_constexpr_literal(std::string_view token)
    {
        if (token.empty())
        {
            throw std::invalid_argument("string_math: empty literal");
        }

        std::string_view text = token;
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

        const auto matches_suffix = [](std::string_view value, std::string_view suffix) constexpr {
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

        const auto consume_fixed_suffix = [&](std::string_view suffix, fixed_integral_suffix kind) constexpr {
            if (matches_suffix(text, suffix))
            {
                fixed_suffix = kind;
                text.remove_suffix(suffix.size());
                return true;
            }
            return false;
        };

        if (text.back() == 'f' || text.back() == 'F')
        {
            text.remove_suffix(1);
            return MathValue(static_cast<float>(parse_decimal_long_double(text)));
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
            throw std::invalid_argument("string_math: invalid constexpr literal suffix");
        }

        if (is_floating_literal)
        {
            if (fixed_suffix != fixed_integral_suffix::None || unsigned_suffix || long_suffix_count == 2)
            {
                throw std::invalid_argument("string_math: invalid constexpr floating literal suffix");
            }
            if (long_suffix_count == 1)
            {
                return MathValue(parse_decimal_long_double(text));
            }
            return MathValue(static_cast<double>(parse_decimal_long_double(text)));
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
            throw std::invalid_argument("string_math: invalid constexpr literal");
        }

        if (fixed_suffix != fixed_integral_suffix::None)
        {
            switch (fixed_suffix)
            {
            case fixed_integral_suffix::I8: return MathValue(parse_integral_constexpr<std::int8_t>(text, base));
            case fixed_integral_suffix::U8: return MathValue(parse_integral_constexpr<std::uint8_t>(text, base));
            case fixed_integral_suffix::I16: return MathValue(parse_integral_constexpr<std::int16_t>(text, base));
            case fixed_integral_suffix::U16: return MathValue(parse_integral_constexpr<std::uint16_t>(text, base));
            case fixed_integral_suffix::I32: return MathValue(parse_integral_constexpr<std::int32_t>(text, base));
            case fixed_integral_suffix::U32: return MathValue(parse_integral_constexpr<std::uint32_t>(text, base));
            case fixed_integral_suffix::I64: return MathValue(parse_integral_constexpr<std::int64_t>(text, base));
            case fixed_integral_suffix::U64: return MathValue(parse_integral_constexpr<std::uint64_t>(text, base));
            case fixed_integral_suffix::Z:
                return MathValue(parse_integral_constexpr<std::make_signed_t<std::size_t>>(text, base));
            case fixed_integral_suffix::UZ: return MathValue(parse_integral_constexpr<std::size_t>(text, base));
            case fixed_integral_suffix::None: break;
            }
        }

        if (unsigned_suffix)
        {
            if (long_suffix_count == 2)
            {
                return MathValue(parse_integral_constexpr<unsigned long long>(text, base));
            }
            if (long_suffix_count == 1)
            {
                return MathValue(parse_integral_constexpr<unsigned long>(text, base));
            }

            const unsigned long long value = parse_integral_constexpr<unsigned long long>(text, base);
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
            return MathValue(parse_integral_constexpr<long long>(text, base));
        }
        if (long_suffix_count == 1)
        {
            return MathValue(parse_integral_constexpr<long>(text, base));
        }

        const unsigned long long value = parse_integral_constexpr<unsigned long long>(text, base);
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

    constexpr MathValue m_apply_infix(std::string_view symbol, const MathValue& left, const MathValue& right)
    {
        if (symbol == "+")
        {
            return left + right;
        }
        if (symbol == "-")
        {
            return left - right;
        }
        if (symbol == "*")
        {
            return left * right;
        }
        if (symbol == "/")
        {
            return left / right;
        }
        if (symbol == "%")
        {
            return left % right;
        }
        if (symbol == "^")
        {
            return MathValue(std::pow(left.to_double(), right.to_double()));
        }
        if (symbol == "max")
        {
            return left > right ? left : right;
        }
        if (symbol == "min")
        {
            return left < right ? left : right;
        }
        if (symbol == "log")
        {
            return MathValue(std::log(right.to_double()) / std::log(left.to_double()));
        }
        if (symbol == "==")
        {
            return MathValue(left == right ? 1 : 0);
        }
        if (symbol == "!=")
        {
            return MathValue(left != right ? 1 : 0);
        }
        if (symbol == "<")
        {
            return MathValue(left < right ? 1 : 0);
        }
        if (symbol == "<=")
        {
            return MathValue(left <= right ? 1 : 0);
        }
        if (symbol == ">")
        {
            return MathValue(left > right ? 1 : 0);
        }
        if (symbol == ">=")
        {
            return MathValue(left >= right ? 1 : 0);
        }
        if (symbol == "&&")
        {
            return MathValue(m_truthy(left) && m_truthy(right) ? 1 : 0);
        }
        if (symbol == "||")
        {
            return MathValue(m_truthy(left) || m_truthy(right) ? 1 : 0);
        }
        throw std::invalid_argument("string_math: unsupported constexpr operator");
    }

    constexpr bool m_truthy(const MathValue& value) const
    {
        return value.visit([](const auto& current) { return current != 0; });
    }

    std::string_view m_expression;
    std::size_t m_position{ 0 };
};

} // namespace string_math::detail
