#pragma once

#include <array>
#include <optional>

#include <string_math/detail/constexpr.hpp>
#include <string_math/semantics.hpp>

namespace string_math
{

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
struct ConstexprVariableEntry
{
    std::string_view name;
    MathValue value;
};

struct ConstexprInfixOperatorEntry
{
    std::string_view symbol;
    int precedence{0};
    Associativity associativity{Associativity::Left};
    MathValue (*invoke)(const MathValue&, const MathValue&) = nullptr;
    CallableSemantics semantics{};
};

struct ConstexprPrefixOperatorEntry
{
    std::string_view symbol;
    int precedence{Precedence::Prefix};
    MathValue (*invoke)(const MathValue&) = nullptr;
    CallableSemantics semantics{};
};

struct ConstexprPostfixOperatorEntry
{
    std::string_view symbol;
    int precedence{Precedence::Postfix};
    MathValue (*invoke)(const MathValue&) = nullptr;
    CallableSemantics semantics{};
};

struct ConstexprFunctionEntry
{
    std::string_view name;
    MathValue (*invoke)(const MathValue*, std::size_t) = nullptr;
    CallableSemantics semantics{};
};

template <
    std::size_t VariableCount = 0,
    std::size_t InfixCount = 0,
    std::size_t FunctionCount = 0,
    std::size_t PrefixCount = 0,
    std::size_t PostfixCount = 0>
class ConstexprContext
{
public:
    static constexpr bool is_constexpr_context = true;

    constexpr ConstexprContext() = default;

    template <detail::fixed_string Name, class T>
    constexpr auto with_value(T value) const
    {
        ConstexprContext<VariableCount + 1, InfixCount, FunctionCount, PrefixCount, PostfixCount> next;
        for (std::size_t index = 0; index < VariableCount; ++index)
        {
            next.m_variables[index] = m_variables[index];
        }
        for (std::size_t index = 0; index < InfixCount; ++index)
        {
            next.m_infix_operators[index] = m_infix_operators[index];
        }
        for (std::size_t index = 0; index < FunctionCount; ++index)
        {
            next.m_functions[index] = m_functions[index];
        }
        for (std::size_t index = 0; index < PrefixCount; ++index)
        {
            next.m_prefix_operators[index] = m_prefix_operators[index];
        }
        for (std::size_t index = 0; index < PostfixCount; ++index)
        {
            next.m_postfix_operators[index] = m_postfix_operators[index];
        }
        next.m_variables[VariableCount] = ConstexprVariableEntry{Name.view(), MathValue(value)};
        return next;
    }

    template <detail::fixed_string Symbol, auto Function>
    constexpr auto with_infix_operator(
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        ConstexprContext<VariableCount, InfixCount + 1, FunctionCount, PrefixCount, PostfixCount> next;
        for (std::size_t index = 0; index < VariableCount; ++index)
        {
            next.m_variables[index] = m_variables[index];
        }
        for (std::size_t index = 0; index < InfixCount; ++index)
        {
            next.m_infix_operators[index] = m_infix_operators[index];
        }
        for (std::size_t index = 0; index < FunctionCount; ++index)
        {
            next.m_functions[index] = m_functions[index];
        }
        for (std::size_t index = 0; index < PrefixCount; ++index)
        {
            next.m_prefix_operators[index] = m_prefix_operators[index];
        }
        for (std::size_t index = 0; index < PostfixCount; ++index)
        {
            next.m_postfix_operators[index] = m_postfix_operators[index];
        }
        next.m_infix_operators[InfixCount] =
            ConstexprInfixOperatorEntry{Symbol.view(), precedence, associativity, &m_binary_invoke<Function>, semantics};
        return next;
    }

    template <detail::fixed_string Name, auto Function>
    constexpr auto with_function(CallableSemantics semantics = {}) const
    {
        ConstexprContext<VariableCount, InfixCount, FunctionCount + 1, PrefixCount, PostfixCount> next;
        for (std::size_t index = 0; index < VariableCount; ++index)
        {
            next.m_variables[index] = m_variables[index];
        }
        for (std::size_t index = 0; index < InfixCount; ++index)
        {
            next.m_infix_operators[index] = m_infix_operators[index];
        }
        for (std::size_t index = 0; index < FunctionCount; ++index)
        {
            next.m_functions[index] = m_functions[index];
        }
        for (std::size_t index = 0; index < PrefixCount; ++index)
        {
            next.m_prefix_operators[index] = m_prefix_operators[index];
        }
        for (std::size_t index = 0; index < PostfixCount; ++index)
        {
            next.m_postfix_operators[index] = m_postfix_operators[index];
        }
        next.m_functions[FunctionCount] = ConstexprFunctionEntry{Name.view(), &m_function_invoke<Function>, semantics};
        return next;
    }

    template <detail::fixed_string Symbol, auto Function>
    constexpr auto with_prefix_operator(int precedence = Precedence::Prefix, CallableSemantics semantics = {}) const
    {
        ConstexprContext<VariableCount, InfixCount, FunctionCount, PrefixCount + 1, PostfixCount> next;
        for (std::size_t index = 0; index < VariableCount; ++index)
        {
            next.m_variables[index] = m_variables[index];
        }
        for (std::size_t index = 0; index < InfixCount; ++index)
        {
            next.m_infix_operators[index] = m_infix_operators[index];
        }
        for (std::size_t index = 0; index < FunctionCount; ++index)
        {
            next.m_functions[index] = m_functions[index];
        }
        for (std::size_t index = 0; index < PrefixCount; ++index)
        {
            next.m_prefix_operators[index] = m_prefix_operators[index];
        }
        for (std::size_t index = 0; index < PostfixCount; ++index)
        {
            next.m_postfix_operators[index] = m_postfix_operators[index];
        }
        next.m_prefix_operators[PrefixCount] =
            ConstexprPrefixOperatorEntry{Symbol.view(), precedence, &m_unary_invoke<Function>, semantics};
        return next;
    }

    template <detail::fixed_string Symbol, auto Function>
    constexpr auto with_postfix_operator(int precedence = Precedence::Postfix, CallableSemantics semantics = {}) const
    {
        ConstexprContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount + 1> next;
        for (std::size_t index = 0; index < VariableCount; ++index)
        {
            next.m_variables[index] = m_variables[index];
        }
        for (std::size_t index = 0; index < InfixCount; ++index)
        {
            next.m_infix_operators[index] = m_infix_operators[index];
        }
        for (std::size_t index = 0; index < FunctionCount; ++index)
        {
            next.m_functions[index] = m_functions[index];
        }
        for (std::size_t index = 0; index < PrefixCount; ++index)
        {
            next.m_prefix_operators[index] = m_prefix_operators[index];
        }
        for (std::size_t index = 0; index < PostfixCount; ++index)
        {
            next.m_postfix_operators[index] = m_postfix_operators[index];
        }
        next.m_postfix_operators[PostfixCount] =
            ConstexprPostfixOperatorEntry{Symbol.view(), precedence, &m_unary_invoke<Function>, semantics};
        return next;
    }

    constexpr std::optional<MathValue> find_value(std::string_view name) const
    {
        for (std::size_t index = 0; index < VariableCount; ++index)
        {
            if (m_variables[index].name == name)
            {
                return m_variables[index].value;
            }
        }
        return std::nullopt;
    }

    constexpr std::string_view match_infix_symbol(std::string_view text) const
    {
        std::string_view best;
        for (std::size_t index = 0; index < InfixCount; ++index)
        {
            const auto symbol = m_infix_operators[index].symbol;
            if (text.size() >= symbol.size() && text.substr(0, symbol.size()) == symbol && symbol.size() > best.size())
            {
                best = symbol;
            }
        }
        return best;
    }

    constexpr std::string_view match_prefix_symbol(std::string_view text) const
    {
        std::string_view best;
        for (std::size_t index = 0; index < PrefixCount; ++index)
        {
            const auto symbol = m_prefix_operators[index].symbol;
            if (text.size() >= symbol.size() && text.substr(0, symbol.size()) == symbol && symbol.size() > best.size())
            {
                best = symbol;
            }
        }
        return best;
    }

    constexpr std::string_view match_postfix_symbol(std::string_view text) const
    {
        std::string_view best;
        for (std::size_t index = 0; index < PostfixCount; ++index)
        {
            const auto symbol = m_postfix_operators[index].symbol;
            if (text.size() >= symbol.size() && text.substr(0, symbol.size()) == symbol && symbol.size() > best.size())
            {
                best = symbol;
            }
        }
        return best;
    }

    constexpr const ConstexprInfixOperatorEntry* find_infix_operator(std::string_view symbol) const
    {
        for (std::size_t index = 0; index < InfixCount; ++index)
        {
            if (m_infix_operators[index].symbol == symbol)
            {
                return &m_infix_operators[index];
            }
        }
        return nullptr;
    }

    constexpr const ConstexprPrefixOperatorEntry* find_prefix_operator(std::string_view symbol) const
    {
        for (std::size_t index = 0; index < PrefixCount; ++index)
        {
            if (m_prefix_operators[index].symbol == symbol)
            {
                return &m_prefix_operators[index];
            }
        }
        return nullptr;
    }

    constexpr const ConstexprPostfixOperatorEntry* find_postfix_operator(std::string_view symbol) const
    {
        for (std::size_t index = 0; index < PostfixCount; ++index)
        {
            if (m_postfix_operators[index].symbol == symbol)
            {
                return &m_postfix_operators[index];
            }
        }
        return nullptr;
    }

    constexpr const ConstexprFunctionEntry* find_function(std::string_view name) const
    {
        for (std::size_t index = 0; index < FunctionCount; ++index)
        {
            if (m_functions[index].name == name)
            {
                return &m_functions[index];
            }
        }
        return nullptr;
    }

private:
    template <auto Function>
    static constexpr MathValue m_unary_invoke(const MathValue& value)
    {
        return m_unary_invoke_impl<Function>(value);
    }

    template <auto Function>
    static constexpr MathValue m_binary_invoke(const MathValue& left, const MathValue& right)
    {
        return m_binary_invoke_impl<Function>(left, right);
    }

    template <auto Function>
    static constexpr MathValue m_function_invoke(const MathValue* arguments, std::size_t count)
    {
        using signature = detail::signature_traits<detail::remove_cvref_t<decltype(Function)>>;
        if constexpr (signature::arity == 1)
        {
            if (count != 1)
            {
                throw std::invalid_argument("string_math: unsupported constexpr function arity");
            }
            return m_unary_invoke_impl<Function>(arguments[0]);
        }
        else if constexpr (signature::arity == 2)
        {
            if (count != 2)
            {
                throw std::invalid_argument("string_math: unsupported constexpr function arity");
            }
            return m_binary_invoke_impl<Function>(arguments[0], arguments[1]);
        }
        else
        {
            throw std::invalid_argument("string_math: unsupported constexpr function arity");
        }
    }

    template <auto Function>
    static constexpr MathValue m_unary_invoke_impl(const MathValue& value)
    {
        using signature = detail::signature_traits<detail::remove_cvref_t<decltype(Function)>>;
        using argument_t = std::tuple_element_t<0, typename signature::args_tuple>;
        return MathValue(Function(value.template cast<argument_t>()));
    }

    template <auto Function>
    static constexpr MathValue m_binary_invoke_impl(const MathValue& left, const MathValue& right)
    {
        using signature = detail::signature_traits<detail::remove_cvref_t<decltype(Function)>>;
        using left_t = std::tuple_element_t<0, typename signature::args_tuple>;
        using right_t = std::tuple_element_t<1, typename signature::args_tuple>;
        return MathValue(Function(left.template cast<left_t>(), right.template cast<right_t>()));
    }

    std::array<ConstexprVariableEntry, VariableCount> m_variables{};
    std::array<ConstexprInfixOperatorEntry, InfixCount> m_infix_operators{};
    std::array<ConstexprFunctionEntry, FunctionCount> m_functions{};
    std::array<ConstexprPrefixOperatorEntry, PrefixCount> m_prefix_operators{};
    std::array<ConstexprPostfixOperatorEntry, PostfixCount> m_postfix_operators{};

    template <std::size_t, std::size_t, std::size_t, std::size_t, std::size_t>
    friend class ConstexprContext;
};
#endif

} // namespace string_math
