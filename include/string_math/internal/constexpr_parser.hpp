#pragma once
// Do not include this file directly. Use <string_math/string_math.hpp> or individual public headers.

#include <array>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

#include <string_math/internal/overload_impl.hpp>
#include <string_math/internal/parser.hpp>
#include <string_math/internal/static_context_data.hpp>
#include <string_math/value/value.hpp>

namespace string_math::internal
{

template <class Context = void>
class ConstexprParser
{
public:
    constexpr explicit ConstexprParser(std::string_view expression, const Context* context = nullptr)
        : m_expression(expression),
          m_context(context)
    {}

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
        int precedence{0};
        Associativity associativity{Associativity::Left};
    };

    struct MatchedUnaryOperator
    {
        std::string_view symbol;
        int precedence{0};
    };

    static constexpr bool m_has_arity(std::span<const CallableOverloadView> overloads, std::size_t arity)
    {
        for (const CallableOverloadView& overload : overloads)
        {
            if (overload.accepts_arity(arity))
            {
                return true;
            }
        }
        return false;
    }

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
            if (m_position >= m_expression.size() || m_expression[m_position] == ')' || m_expression[m_position] == ',')
            {
                break;
            }

            const MatchedUnaryOperator postfix = m_match_postfix();
            if (!postfix.symbol.empty() && postfix.precedence >= min_precedence)
            {
                m_position += postfix.symbol.size();
                left = m_apply_postfix(postfix.symbol, left);
                continue;
            }

            const MatchedOperator infix = m_match_infix();
            if (infix.symbol.empty() || infix.precedence < min_precedence)
            {
                break;
            }

            m_position += infix.symbol.size();
            const int next_min = infix.associativity == Associativity::Left ? infix.precedence + 1 : infix.precedence;
            MathValue right = m_parse_expression(next_min);
            left = m_apply_infix(infix.symbol, left, right);
        }

        m_skip_spaces();
        if (min_precedence <= k_conditional_precedence && m_position < m_expression.size() && m_expression[m_position] == '?')
        {
            ++m_position;
            const MathValue true_value = m_parse_expression(0);
            m_skip_spaces();
            if (m_position >= m_expression.size() || m_expression[m_position] != ':')
            {
                throw std::invalid_argument("string_math: expected ':' in conditional expression");
            }
            ++m_position;
            const MathValue false_value = m_parse_expression(k_conditional_precedence);
            return m_truthy(left) ? true_value : false_value;
        }

        return left;
    }

    constexpr MatchedOperator m_match_infix() const noexcept
    {
        if constexpr (!std::is_same_v<Context, void>)
        {
            if (m_context != nullptr)
            {
                const std::string_view tail = m_expression.substr(m_position);
                const std::string_view symbol = m_context->match_infix_symbol(tail);
                if (!symbol.empty())
                {
                    if (const auto* entry = m_context->find_infix_operator(symbol); entry != nullptr)
                    {
                        return {entry->name_or_symbol, entry->precedence, entry->associativity};
                    }
                }
            }
        }
        return {};
    }

    constexpr MatchedUnaryOperator m_match_prefix() const noexcept
    {
        if constexpr (!std::is_same_v<Context, void>)
        {
            if (m_context != nullptr)
            {
                const std::string_view tail = m_expression.substr(m_position);
                const std::string_view symbol = m_context->match_prefix_symbol(tail);
                if (!symbol.empty())
                {
                    if (const auto* entry = m_context->find_prefix_operator(symbol); entry != nullptr)
                    {
                        return {entry->name_or_symbol, entry->precedence};
                    }
                }
            }
        }
        return {};
    }

    constexpr MatchedUnaryOperator m_match_postfix() const noexcept
    {
        if constexpr (!std::is_same_v<Context, void>)
        {
            if (m_context != nullptr)
            {
                const std::string_view tail = m_expression.substr(m_position);
                const std::string_view symbol = m_context->match_postfix_symbol(tail);
                if (!symbol.empty())
                {
                    if (const auto* entry = m_context->find_postfix_operator(symbol); entry != nullptr)
                    {
                        return {entry->name_or_symbol, entry->precedence};
                    }
                }
            }
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

        const MatchedUnaryOperator prefix = m_match_prefix();
        if (!prefix.symbol.empty())
        {
            m_position += prefix.symbol.size();
            const MathValue value = m_parse_expression(prefix.precedence);
            return m_apply_prefix(prefix.symbol, value);
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

        if (m_expression[m_position] == '{')
        {
            ++m_position;
            const std::string_view name = m_parse_variable_name();
            if constexpr (!std::is_same_v<Context, void>)
            {
                if (m_context != nullptr)
                {
                    if (const auto value = m_context->find_value(name))
                    {
                        return *value;
                    }
                }
            }
            throw std::invalid_argument("string_math: unknown constexpr variable");
        }

        if (is_identifier_start(m_expression[m_position]))
        {
            const std::string_view identifier = m_parse_identifier();
            m_skip_spaces();
            if (m_position < m_expression.size() && m_expression[m_position] == '(')
            {
                return m_parse_function_call(identifier);
            }

            if constexpr (!std::is_same_v<Context, void>)
            {
                if (m_context != nullptr)
                {
                    const auto callable = m_context->lookup_function(identifier);
                    if (callable && m_has_arity(callable.overloads, 1))
                    {
                        const MathValue argument = m_parse_expression(k_implicit_prefix_call_precedence);
                        return m_apply_function(identifier, &argument, 1);
                    }
                }
            }

            throw std::invalid_argument("string_math: unknown constexpr identifier");
        }

        return m_parse_constexpr_literal(m_parse_literal());
    }

    constexpr std::string_view m_parse_identifier()
    {
        const std::size_t begin = m_position++;
        while (m_position < m_expression.size() && is_identifier_char(m_expression[m_position]))
        {
            ++m_position;
        }
        return m_expression.substr(begin, m_position - begin);
    }

    constexpr std::string_view m_parse_variable_name()
    {
        const std::size_t begin = m_position;
        while (m_position < m_expression.size() && m_expression[m_position] != '}')
        {
            ++m_position;
        }
        if (m_position >= m_expression.size())
        {
            throw std::invalid_argument("string_math: expected '}'");
        }

        std::size_t trimmed_begin = begin;
        std::size_t trimmed_end = m_position;
        while (trimmed_begin < trimmed_end && is_space(m_expression[trimmed_begin]))
        {
            ++trimmed_begin;
        }
        while (trimmed_end > trimmed_begin && is_space(m_expression[trimmed_end - 1]))
        {
            --trimmed_end;
        }

        const std::string_view name = m_expression.substr(trimmed_begin, trimmed_end - trimmed_begin);
        ++m_position;
        return name;
    }

    constexpr MathValue m_parse_function_call(std::string_view name)
    {
        ++m_position;
        std::vector<MathValue> arguments;
        m_skip_spaces();
        if (m_position < m_expression.size() && m_expression[m_position] != ')')
        {
            for (;;)
            {
                arguments.push_back(m_parse_expression(0));
                m_skip_spaces();
                if (m_position >= m_expression.size() || m_expression[m_position] != ',')
                {
                    break;
                }
                ++m_position;
                m_skip_spaces();
            }
        }

        if (m_position >= m_expression.size() || m_expression[m_position] != ')')
        {
            throw std::invalid_argument("string_math: expected ')' after constexpr function call");
        }
        ++m_position;

        return m_apply_function(name, arguments.data(), arguments.size());
    }

    constexpr std::string_view m_parse_literal()
    {
        const std::size_t begin = m_position;
        while (m_position < m_expression.size())
        {
            const char ch = m_expression[m_position];
            if (is_space(ch) || ch == ')' || ch == ',' || ch == '{' || ch == '}')
            {
                break;
            }

            if constexpr (!std::is_same_v<Context, void>)
            {
                if (m_context != nullptr && m_position != begin)
                {
                    const std::string_view tail = m_expression.substr(m_position);
                    const std::string_view infix = m_context->match_infix_symbol(tail);
                    const std::string_view postfix = m_context->match_postfix_symbol(tail);
                    if (!infix.empty() || !postfix.empty())
                    {
                        const char previous = m_expression[m_position - 1];
                        const bool signed_exponent = (ch == '+' || ch == '-') && (previous == 'e' || previous == 'E');
                        const bool literal_alpha = is_identifier_char(ch) || ch == '.';
                        if (!signed_exponent && !literal_alpha)
                        {
                            break;
                        }
                    }
                }
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

        const bool is_floating_literal =
            text.find('.') != std::string_view::npos || text.find('e') != std::string_view::npos || text.find('E') != std::string_view::npos;

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
        const std::array<MathValue, 2> arguments{left, right};
        return m_apply_binary_callable(symbol, m_context->lookup_infix_operator(symbol), arguments);
    }

    constexpr MathValue m_apply_prefix(std::string_view symbol, const MathValue& value)
    {
        const std::array<MathValue, 1> arguments{value};
        return m_apply_unary_callable(symbol, m_context->lookup_prefix_operator(symbol), arguments);
    }

    constexpr MathValue m_apply_postfix(std::string_view symbol, const MathValue& value)
    {
        const std::array<MathValue, 1> arguments{value};
        return m_apply_unary_callable(symbol, m_context->lookup_postfix_operator(symbol), arguments);
    }

    constexpr MathValue m_apply_function(std::string_view name, const MathValue* arguments, std::size_t count)
    {
        if constexpr (std::is_same_v<Context, void>)
        {
            (void)name;
            (void)arguments;
            (void)count;
            throw std::invalid_argument("string_math: unsupported constexpr function");
        }
        else
        {
            if (m_context == nullptr)
            {
                throw std::invalid_argument("string_math: unsupported constexpr function");
            }

            const auto callable = m_context->lookup_function(name);
            if (!callable)
            {
                throw std::invalid_argument("string_math: unsupported constexpr function");
            }

            std::vector<ValueType> argument_types;
            argument_types.reserve(count);
            for (std::size_t index = 0; index < count; ++index)
            {
                argument_types.push_back(arguments[index].type());
            }

            const OverloadResolutionMatch match = resolve_overload_match(callable.overloads, argument_types, m_context->policy().promotion);
            if (match.overload == nullptr)
            {
                throw std::invalid_argument("string_math: no matching overload for constexpr function");
            }
            if (match.ambiguous)
            {
                throw std::invalid_argument("string_math: ambiguous constexpr function overload");
            }

            return invoke_constexpr_overload(*match.overload, arguments, count, m_context->policy(), name);
        }
    }

    template <std::size_t Arity>
    constexpr MathValue m_apply_unary_callable(
        std::string_view symbol,
        const CollectedCallable& callable,
        const std::array<MathValue, Arity>& arguments)
    {
        return m_apply_fixed_callable(symbol, callable, arguments);
    }

    template <std::size_t Arity>
    constexpr MathValue m_apply_binary_callable(
        std::string_view symbol,
        const CollectedCallable& callable,
        const std::array<MathValue, Arity>& arguments)
    {
        return m_apply_fixed_callable(symbol, callable, arguments);
    }

    template <std::size_t Arity>
    constexpr MathValue m_apply_fixed_callable(
        std::string_view symbol,
        const CollectedCallable& callable,
        const std::array<MathValue, Arity>& arguments)
    {
        if constexpr (std::is_same_v<Context, void>)
        {
            (void)symbol;
            (void)callable;
            (void)arguments;
            throw std::invalid_argument("string_math: unsupported constexpr operator");
        }
        else
        {
            if (m_context == nullptr || !callable)
            {
                throw std::invalid_argument("string_math: unsupported constexpr operator");
            }

            std::array<ValueType, Arity> argument_types{};
            for (std::size_t index = 0; index < Arity; ++index)
            {
                argument_types[index] = arguments[index].type();
            }

            const OverloadResolutionMatch match = resolve_overload_match(callable.overloads, argument_types, m_context->policy().promotion);
            if (match.overload == nullptr)
            {
                throw std::invalid_argument("string_math: no matching overload for constexpr operator");
            }
            if (match.ambiguous)
            {
                throw std::invalid_argument("string_math: ambiguous constexpr operator overload");
            }

            return invoke_constexpr_overload(*match.overload, arguments.data(), Arity, m_context->policy(), symbol);
        }
    }

    constexpr bool m_truthy(const MathValue& value) const
    {
        return value.visit([](const auto& current) { return current != 0; });
    }

    std::string_view m_expression;
    std::size_t m_position{0};
    const Context* m_context{nullptr};
};

} // namespace string_math::internal
