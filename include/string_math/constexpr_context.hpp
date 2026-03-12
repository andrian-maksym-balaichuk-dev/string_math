#pragma once

#include <array>
#include <optional>

#include <string_math/detail/constexpr.hpp>
#include <string_math/semantics.hpp>

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
namespace string_math::detail
{

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
    std::size_t VariableCount,
    std::size_t InfixCount,
    std::size_t FunctionCount,
    std::size_t PrefixCount,
    std::size_t PostfixCount>
class StaticMathContext
{
public:
    static constexpr bool is_constexpr_context = true;

    constexpr StaticMathContext() = default;

    template <std::size_t NameSize, class T>
    constexpr auto with_value(const char (&name)[NameSize], T value) const
    {
        return m_with_value(std::string_view(name, NameSize - 1), MathValue(value));
    }

    template <std::size_t NameSize, class T>
    constexpr auto set_value(const char (&name)[NameSize], T value) const
    {
        return with_value(name, value);
    }

    template <std::size_t NameSize, class T>
    constexpr auto add_variable(const char (&name)[NameSize], T value) const
    {
        return with_value(name, value);
    }

    template <std::size_t NameSize, class T>
    constexpr auto register_variable(const char (&name)[NameSize], T value) const
    {
        return with_value(name, value);
    }

    constexpr auto detail_with_value_entry(std::string_view name, MathValue value) const
    {
        return m_with_value(name, value);
    }

    constexpr auto detail_with_infix_entry(
        std::string_view symbol,
        int precedence,
        Associativity associativity,
        MathValue (*invoke)(const MathValue&, const MathValue&),
        CallableSemantics semantics = {}) const
    {
        return m_with_infix_operator(symbol, precedence, associativity, invoke, semantics);
    }

    constexpr auto detail_with_prefix_entry(
        std::string_view symbol,
        int precedence,
        MathValue (*invoke)(const MathValue&),
        CallableSemantics semantics = {}) const
    {
        return m_with_prefix_operator(symbol, precedence, invoke, semantics);
    }

    constexpr auto detail_with_postfix_entry(
        std::string_view symbol,
        int precedence,
        MathValue (*invoke)(const MathValue&),
        CallableSemantics semantics = {}) const
    {
        return m_with_postfix_operator(symbol, precedence, invoke, semantics);
    }

    constexpr auto detail_with_function_entry(
        std::string_view name,
        MathValue (*invoke)(const MathValue*, std::size_t),
        CallableSemantics semantics = {}) const
    {
        return m_with_function(name, invoke, semantics);
    }

    template <class Sig, auto Function, std::size_t SymbolSize>
    constexpr auto with_infix_operator(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        static_assert(
            std::is_same_v<detail::remove_cvref_t<Sig>, detail::remove_cvref_t<fw::detail::fn_sig_t<decltype(Function)>>>,
            "string_math: Sig must match the compile-time infix function signature");
        return m_with_infix_operator(
            std::string_view(symbol, SymbolSize - 1), precedence, associativity, &m_binary_invoke<Function>, semantics);
    }

    template <auto Function, std::size_t SymbolSize>
    constexpr auto with_infix_operator(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return with_infix_operator<fw::detail::fn_sig_t<decltype(Function)>, Function>(
            symbol, precedence, associativity, semantics);
    }

    template <class Sig, auto Function, std::size_t SymbolSize>
    constexpr auto add_infix_operator(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return with_infix_operator<Sig, Function>(symbol, precedence, associativity, semantics);
    }

    template <auto Function, std::size_t SymbolSize>
    constexpr auto add_infix_operator(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return with_infix_operator<Function>(symbol, precedence, associativity, semantics);
    }

    template <class Sig, auto Function, std::size_t SymbolSize>
    constexpr auto register_infix_operator(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return with_infix_operator<Sig, Function>(symbol, precedence, associativity, semantics);
    }

    template <auto Function, std::size_t SymbolSize>
    constexpr auto register_infix_operator(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return with_infix_operator<Function>(symbol, precedence, associativity, semantics);
    }

    template <class Sig, auto Function, std::size_t NameSize>
    constexpr auto with_function(const char (&name)[NameSize], CallableSemantics semantics = {}) const
    {
        static_assert(
            std::is_same_v<detail::remove_cvref_t<Sig>, detail::remove_cvref_t<fw::detail::fn_sig_t<decltype(Function)>>>,
            "string_math: Sig must match the compile-time function signature");
        return m_with_function(std::string_view(name, NameSize - 1), &m_function_invoke<Function>, semantics);
    }

    template <auto Function, std::size_t NameSize>
    constexpr auto with_function(const char (&name)[NameSize], CallableSemantics semantics = {}) const
    {
        return with_function<fw::detail::fn_sig_t<decltype(Function)>, Function>(name, semantics);
    }

    template <class Sig, auto Function, std::size_t NameSize>
    constexpr auto add_function(const char (&name)[NameSize], CallableSemantics semantics = {}) const
    {
        return with_function<Sig, Function>(name, semantics);
    }

    template <auto Function, std::size_t NameSize>
    constexpr auto add_function(const char (&name)[NameSize], CallableSemantics semantics = {}) const
    {
        return with_function<Function>(name, semantics);
    }

    template <class Sig, auto Function, std::size_t NameSize>
    constexpr auto register_function(const char (&name)[NameSize], CallableSemantics semantics = {}) const
    {
        return with_function<Sig, Function>(name, semantics);
    }

    template <auto Function, std::size_t NameSize>
    constexpr auto register_function(const char (&name)[NameSize], CallableSemantics semantics = {}) const
    {
        return with_function<Function>(name, semantics);
    }

    template <class Sig, auto Function, std::size_t SymbolSize>
    constexpr auto with_prefix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {}) const
    {
        static_assert(
            std::is_same_v<detail::remove_cvref_t<Sig>, detail::remove_cvref_t<fw::detail::fn_sig_t<decltype(Function)>>>,
            "string_math: Sig must match the compile-time prefix function signature");
        return m_with_prefix_operator(
            std::string_view(symbol, SymbolSize - 1), precedence, &m_unary_invoke<Function>, semantics);
    }

    template <auto Function, std::size_t SymbolSize>
    constexpr auto with_prefix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {}) const
    {
        return with_prefix_operator<fw::detail::fn_sig_t<decltype(Function)>, Function>(symbol, precedence, semantics);
    }

    template <class Sig, auto Function, std::size_t SymbolSize>
    constexpr auto add_prefix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {}) const
    {
        return with_prefix_operator<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function, std::size_t SymbolSize>
    constexpr auto add_prefix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {}) const
    {
        return with_prefix_operator<Function>(symbol, precedence, semantics);
    }

    template <class Sig, auto Function, std::size_t SymbolSize>
    constexpr auto register_prefix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {}) const
    {
        return with_prefix_operator<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function, std::size_t SymbolSize>
    constexpr auto register_prefix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {}) const
    {
        return with_prefix_operator<Function>(symbol, precedence, semantics);
    }

    template <class Sig, auto Function, std::size_t SymbolSize>
    constexpr auto with_postfix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {}) const
    {
        static_assert(
            std::is_same_v<detail::remove_cvref_t<Sig>, detail::remove_cvref_t<fw::detail::fn_sig_t<decltype(Function)>>>,
            "string_math: Sig must match the compile-time postfix function signature");
        return m_with_postfix_operator(
            std::string_view(symbol, SymbolSize - 1), precedence, &m_unary_invoke<Function>, semantics);
    }

    template <auto Function, std::size_t SymbolSize>
    constexpr auto with_postfix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {}) const
    {
        return with_postfix_operator<fw::detail::fn_sig_t<decltype(Function)>, Function>(symbol, precedence, semantics);
    }

    template <class Sig, auto Function, std::size_t SymbolSize>
    constexpr auto add_postfix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {}) const
    {
        return with_postfix_operator<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function, std::size_t SymbolSize>
    constexpr auto add_postfix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {}) const
    {
        return with_postfix_operator<Function>(symbol, precedence, semantics);
    }

    template <class Sig, auto Function, std::size_t SymbolSize>
    constexpr auto register_postfix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {}) const
    {
        return with_postfix_operator<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function, std::size_t SymbolSize>
    constexpr auto register_postfix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {}) const
    {
        return with_postfix_operator<Function>(symbol, precedence, semantics);
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
    template <class NextContext>
    constexpr void m_copy_into(NextContext& next) const
    {
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
    }

    constexpr auto m_with_value(std::string_view name, MathValue value) const
    {
        StaticMathContext<VariableCount + 1, InfixCount, FunctionCount, PrefixCount, PostfixCount> next;
        m_copy_into(next);
        next.m_variables[VariableCount] = ConstexprVariableEntry{name, value};
        return next;
    }

    constexpr auto m_with_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity,
        MathValue (*invoke)(const MathValue&, const MathValue&),
        CallableSemantics semantics) const
    {
        StaticMathContext<VariableCount, InfixCount + 1, FunctionCount, PrefixCount, PostfixCount> next;
        m_copy_into(next);
        next.m_infix_operators[InfixCount] = ConstexprInfixOperatorEntry{symbol, precedence, associativity, invoke, semantics};
        return next;
    }

    constexpr auto m_with_function(
        std::string_view name,
        MathValue (*invoke)(const MathValue*, std::size_t),
        CallableSemantics semantics) const
    {
        StaticMathContext<VariableCount, InfixCount, FunctionCount + 1, PrefixCount, PostfixCount> next;
        m_copy_into(next);
        next.m_functions[FunctionCount] = ConstexprFunctionEntry{name, invoke, semantics};
        return next;
    }

    constexpr auto m_with_prefix_operator(
        std::string_view symbol,
        int precedence,
        MathValue (*invoke)(const MathValue&),
        CallableSemantics semantics) const
    {
        StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount + 1, PostfixCount> next;
        m_copy_into(next);
        next.m_prefix_operators[PrefixCount] = ConstexprPrefixOperatorEntry{symbol, precedence, invoke, semantics};
        return next;
    }

    constexpr auto m_with_postfix_operator(
        std::string_view symbol,
        int precedence,
        MathValue (*invoke)(const MathValue&),
        CallableSemantics semantics) const
    {
        StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount + 1> next;
        m_copy_into(next);
        next.m_postfix_operators[PostfixCount] = ConstexprPostfixOperatorEntry{symbol, precedence, invoke, semantics};
        return next;
    }

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
    friend class StaticMathContext;
};

template <class... Ts, std::size_t VariableCount, std::size_t InfixCount, std::size_t FunctionCount, std::size_t PrefixCount, std::size_t PostfixCount, class F>
constexpr auto register_builtin_infix(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view symbol,
    F&&,
    CallableSemantics semantics)
{
    const auto spec = require_builtin_infix_spec(symbol);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    return context.detail_with_infix_entry(symbol, spec.precedence, spec.associativity, spec.invoke, effective_semantics);
}

template <class... Ts, std::size_t VariableCount, std::size_t InfixCount, std::size_t FunctionCount, std::size_t PrefixCount, std::size_t PostfixCount, class F>
constexpr auto register_builtin_prefix(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view symbol,
    F&&,
    CallableSemantics semantics)
{
    const auto spec = require_builtin_prefix_spec(symbol);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    return context.detail_with_prefix_entry(symbol, spec.precedence, spec.invoke, effective_semantics);
}

template <class... Ts, std::size_t VariableCount, std::size_t InfixCount, std::size_t FunctionCount, std::size_t PrefixCount, std::size_t PostfixCount, class F>
constexpr auto register_builtin_postfix(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view symbol,
    F&&,
    CallableSemantics semantics)
{
    const auto spec = require_builtin_postfix_spec(symbol);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    return context.detail_with_postfix_entry(symbol, spec.precedence, spec.invoke, effective_semantics);
}

template <class... Ts, std::size_t VariableCount, std::size_t InfixCount, std::size_t FunctionCount, std::size_t PrefixCount, std::size_t PostfixCount, class F>
constexpr auto register_builtin_function(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view name,
    F&&,
    CallableSemantics semantics)
{
    const auto spec = require_builtin_function_spec(name);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    return context.detail_with_function_entry(name, spec.invoke, effective_semantics);
}

template <class Sig, std::size_t VariableCount, std::size_t InfixCount, std::size_t FunctionCount, std::size_t PrefixCount, std::size_t PostfixCount, class F>
constexpr auto register_builtin_function_signature(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view name,
    F&&,
    CallableSemantics semantics)
{
    const auto spec = require_builtin_function_spec(name);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    return context.detail_with_function_entry(name, spec.invoke, effective_semantics);
}

template <class... Sigs, std::size_t VariableCount, std::size_t InfixCount, std::size_t FunctionCount, std::size_t PrefixCount, std::size_t PostfixCount, class F>
constexpr auto register_builtin_postfix_overloads(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view symbol,
    F&&,
    CallableSemantics semantics)
{
    const auto spec = require_builtin_postfix_spec(symbol);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    return context.detail_with_postfix_entry(symbol, spec.precedence, spec.invoke, effective_semantics);
}

template <std::size_t VariableCount, std::size_t InfixCount, std::size_t FunctionCount, std::size_t PrefixCount, std::size_t PostfixCount, class F>
constexpr auto register_builtin_variadic_function(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view name,
    std::size_t,
    F&&,
    CallableSemantics semantics)
{
    const auto spec = require_builtin_function_spec(name);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    return context.detail_with_function_entry(name, spec.invoke, effective_semantics);
}

template <std::size_t VariableCount, std::size_t InfixCount, std::size_t FunctionCount, std::size_t PrefixCount, std::size_t PostfixCount>
constexpr auto register_builtin_value(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view name,
    MathValue value)
{
    return context.detail_with_value_entry(name, value);
}

} // namespace string_math::detail
#endif

namespace string_math
{

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
template <
    std::size_t VariableCount,
    std::size_t InfixCount,
    std::size_t FunctionCount,
    std::size_t PrefixCount,
    std::size_t PostfixCount>
constexpr auto MathContext::compile_time()
{
    return detail::StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount>{};
}
#endif

} // namespace string_math
