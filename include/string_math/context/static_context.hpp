#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include <string_math/context/context.hpp>
#include <string_math/internal/overload_impl.hpp>
#include <string_math/internal/static_context_data.hpp>

namespace string_math::internal
{

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION

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

    constexpr auto with_value(std::string_view name, MathValue value) const
    {
        return detail_set_value(name, value);
    }

    template <std::size_t NameSize, class T>
    constexpr auto with_value(const char (&name)[NameSize], T value) const
    {
        return with_value(std::string_view(name, NameSize - 1), MathValue(value));
    }

    constexpr auto set_value(std::string_view name, MathValue value) const
    {
        return detail_set_value(name, value);
    }

    template <std::size_t NameSize, class T>
    constexpr auto set_value(const char (&name)[NameSize], T value) const
    {
        return set_value(std::string_view(name, NameSize - 1), MathValue(value));
    }

    constexpr auto add_variable(std::string_view name, MathValue value) const
    {
        return detail_set_value(name, value);
    }

    template <std::size_t NameSize, class T>
    constexpr auto add_variable(const char (&name)[NameSize], T value) const
    {
        return add_variable(std::string_view(name, NameSize - 1), MathValue(value));
    }

    constexpr auto register_variable(std::string_view name, MathValue value) const
    {
        return detail_set_value(name, value);
    }

    template <std::size_t NameSize, class T>
    constexpr auto register_variable(const char (&name)[NameSize], T value) const
    {
        return register_variable(std::string_view(name, NameSize - 1), MathValue(value));
    }

    template <class Sig, auto Function>
    constexpr auto add_function(std::string_view name, CallableSemantics semantics = {}) const
    {
        return m_append_function(name, m_make_fixed_overload<Sig, Function>(semantics));
    }

    template <auto Function>
    constexpr auto add_function(std::string_view name, CallableSemantics semantics = {}) const
    {
        return add_function<fw::detail::fn_sig_t<decltype(Function)>, Function>(name, semantics);
    }

    template <class Sig, class F>
    constexpr auto add_function(std::string_view name, F function, CallableSemantics semantics = {}) const
    {
        return add_function<fw::detail::fn_sig_t<decltype(+function)>, +function>(name, semantics);
    }

    constexpr auto add_function(std::string_view name, auto function, CallableSemantics semantics = {}) const
    {
        return add_function<fw::detail::fn_sig_t<decltype(+function)>, +function>(name, semantics);
    }

    template <class Sig, auto Function>
    constexpr auto register_function(std::string_view name, CallableSemantics semantics = {}) const
    {
        return add_function<Sig, Function>(name, semantics);
    }

    template <auto Function>
    constexpr auto register_function(std::string_view name, CallableSemantics semantics = {}) const
    {
        return add_function<Function>(name, semantics);
    }

    template <class Sig, class F>
    constexpr auto register_function(std::string_view name, F function, CallableSemantics semantics = {}) const
    {
        return add_function<Sig>(name, function, semantics);
    }

    template <class Sig, class... Rest, class F>
    constexpr auto add_function_overloads(std::string_view name, F function, CallableSemantics semantics = {}) const
    {
        auto next = add_function<Sig>(name, function, semantics);
        if constexpr (sizeof...(Rest) == 0)
        {
            return next;
        }
        else
        {
            return next.template add_function_overloads<Rest...>(name, function, semantics);
        }
    }

    template <class... Ts, class F>
    constexpr auto add_function_for(std::string_view name, F function, CallableSemantics semantics = {}) const
    {
        return add_function_overloads<typename unary_signature_for<Ts, decltype(function)>::type...>(name, function, semantics);
    }

    template <class... Ts, class F>
    constexpr auto add_binary_function_for(std::string_view name, F function, CallableSemantics semantics = {}) const
    {
        return add_function_overloads<typename binary_signature_for<Ts, decltype(function)>::type...>(name, function, semantics);
    }

    constexpr auto add_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        callable_invoke_fn invoke,
        ValueType result_type,
        CallableSemantics semantics = {}) const
    {
        return add_variadic_function(name, min_arity, k_unbounded_arity, invoke, result_type, semantics);
    }

    constexpr auto add_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        callable_invoke_fn invoke,
        ValueType result_type,
        CallableSemantics semantics = {}) const
    {
        return m_append_function(
            name,
            CallableOverloadView{
                result_type,
                ArityKind::Variadic,
                min_arity,
                max_arity,
                {},
                semantics,
                nullptr,
                invoke,
                nullptr,
                nullptr,
            });
    }

    constexpr auto add_generic_function(
        std::string_view name,
        callable_invoke_fn invoke,
        std::size_t min_arity,
        std::size_t max_arity,
        ValueType result_type = ValueType::Double,
        CallableSemantics semantics = {},
        callable_policy_fn policy_invoke = nullptr) const
    {
        return m_append_function(
            name,
            CallableOverloadView{
                result_type,
                max_arity == k_unbounded_arity ? ArityKind::Variadic : ArityKind::Fixed,
                min_arity,
                max_arity,
                {},
                semantics,
                nullptr,
                invoke,
                nullptr,
                policy_invoke,
            });
    }

    constexpr auto add_generic_prefix_operator(
        std::string_view symbol,
        int precedence,
        callable_invoke_fn invoke,
        ValueType result_type = ValueType::Double,
        CallableSemantics semantics = {},
        callable_policy_fn policy_invoke = nullptr) const
    {
        return m_append_prefix(
            symbol,
            precedence,
            CallableOverloadView{
                result_type,
                ArityKind::Fixed,
                1,
                1,
                {},
                semantics,
                nullptr,
                invoke,
                nullptr,
                policy_invoke,
            });
    }

    constexpr auto add_generic_postfix_operator(
        std::string_view symbol,
        int precedence,
        callable_invoke_fn invoke,
        ValueType result_type = ValueType::Double,
        CallableSemantics semantics = {},
        callable_policy_fn policy_invoke = nullptr) const
    {
        return m_append_postfix(
            symbol,
            precedence,
            CallableOverloadView{
                result_type,
                ArityKind::Fixed,
                1,
                1,
                {},
                semantics,
                nullptr,
                invoke,
                nullptr,
                policy_invoke,
            });
    }

    constexpr auto add_generic_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity,
        callable_invoke_fn invoke,
        ValueType result_type = ValueType::Double,
        CallableSemantics semantics = {},
        callable_policy_fn policy_invoke = nullptr) const
    {
        return m_append_infix(
            symbol,
            precedence,
            associativity,
            CallableOverloadView{
                result_type,
                ArityKind::Fixed,
                2,
                2,
                {},
                semantics,
                nullptr,
                invoke,
                nullptr,
                policy_invoke,
            });
    }

    template <class Sig, auto Function>
    constexpr auto add_prefix_operator(std::string_view symbol, int precedence, CallableSemantics semantics = {}) const
    {
        return m_append_prefix(symbol, precedence, m_make_fixed_overload<Sig, Function>(semantics));
    }

    template <auto Function>
    constexpr auto add_prefix_operator(std::string_view symbol, int precedence, CallableSemantics semantics = {}) const
    {
        return add_prefix_operator<fw::detail::fn_sig_t<decltype(Function)>, Function>(symbol, precedence, semantics);
    }

    template <class Sig, class F>
    constexpr auto add_prefix_operator(std::string_view symbol, F function, int precedence, CallableSemantics semantics = {}) const
    {
        return add_prefix_operator<fw::detail::fn_sig_t<decltype(+function)>, +function>(symbol, precedence, semantics);
    }

    constexpr auto add_prefix_operator(std::string_view symbol, auto function, int precedence, CallableSemantics semantics = {}) const
    {
        return add_prefix_operator<fw::detail::fn_sig_t<decltype(+function)>, +function>(symbol, precedence, semantics);
    }

    template <class Sig, auto Function>
    constexpr auto register_prefix_operator(std::string_view symbol, int precedence, CallableSemantics semantics = {}) const
    {
        return add_prefix_operator<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function>
    constexpr auto register_prefix_operator(std::string_view symbol, int precedence, CallableSemantics semantics = {}) const
    {
        return add_prefix_operator<Function>(symbol, precedence, semantics);
    }

    template <class Sig, class F>
    constexpr auto register_prefix_operator(std::string_view symbol, F function, int precedence, CallableSemantics semantics = {}) const
    {
        return add_prefix_operator<Sig>(symbol, function, precedence, semantics);
    }

    template <class Sig, class... Rest, class F>
    constexpr auto add_prefix_operator_overloads(
        std::string_view symbol,
        F function,
        int precedence,
        CallableSemantics semantics = {}) const
    {
        auto next = add_prefix_operator<Sig>(symbol, function, precedence, semantics);
        if constexpr (sizeof...(Rest) == 0)
        {
            return next;
        }
        else
        {
            return next.template add_prefix_operator_overloads<Rest...>(symbol, function, precedence, semantics);
        }
    }

    template <class... Ts, class F>
    constexpr auto add_prefix_operator_for(
        std::string_view symbol,
        F function,
        int precedence,
        CallableSemantics semantics = {}) const
    {
        return add_prefix_operator_overloads<typename unary_signature_for<Ts, decltype(function)>::type...>(
            symbol,
            function,
            precedence,
            semantics);
    }

    template <class Sig, auto Function>
    constexpr auto add_postfix_operator(std::string_view symbol, int precedence, CallableSemantics semantics = {}) const
    {
        return m_append_postfix(symbol, precedence, m_make_fixed_overload<Sig, Function>(semantics));
    }

    template <auto Function>
    constexpr auto add_postfix_operator(std::string_view symbol, int precedence, CallableSemantics semantics = {}) const
    {
        return add_postfix_operator<fw::detail::fn_sig_t<decltype(Function)>, Function>(symbol, precedence, semantics);
    }

    template <class Sig, class F>
    constexpr auto add_postfix_operator(std::string_view symbol, F function, int precedence, CallableSemantics semantics = {}) const
    {
        return add_postfix_operator<fw::detail::fn_sig_t<decltype(+function)>, +function>(symbol, precedence, semantics);
    }

    constexpr auto add_postfix_operator(std::string_view symbol, auto function, int precedence, CallableSemantics semantics = {}) const
    {
        return add_postfix_operator<fw::detail::fn_sig_t<decltype(+function)>, +function>(symbol, precedence, semantics);
    }

    template <class Sig, auto Function>
    constexpr auto register_postfix_operator(std::string_view symbol, int precedence, CallableSemantics semantics = {}) const
    {
        return add_postfix_operator<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function>
    constexpr auto register_postfix_operator(std::string_view symbol, int precedence, CallableSemantics semantics = {}) const
    {
        return add_postfix_operator<Function>(symbol, precedence, semantics);
    }

    template <class Sig, class F>
    constexpr auto register_postfix_operator(std::string_view symbol, F function, int precedence, CallableSemantics semantics = {}) const
    {
        return add_postfix_operator<Sig>(symbol, function, precedence, semantics);
    }

    template <class Sig, class... Rest, class F>
    constexpr auto add_postfix_operator_overloads(
        std::string_view symbol,
        F function,
        int precedence,
        CallableSemantics semantics = {}) const
    {
        auto next = add_postfix_operator<Sig>(symbol, function, precedence, semantics);
        if constexpr (sizeof...(Rest) == 0)
        {
            return next;
        }
        else
        {
            return next.template add_postfix_operator_overloads<Rest...>(symbol, function, precedence, semantics);
        }
    }

    template <class... Ts, class F>
    constexpr auto add_postfix_operator_for(
        std::string_view symbol,
        F function,
        int precedence,
        CallableSemantics semantics = {}) const
    {
        return add_postfix_operator_overloads<typename unary_signature_for<Ts, decltype(function)>::type...>(
            symbol,
            function,
            precedence,
            semantics);
    }

    template <class Sig, auto Function>
    constexpr auto add_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return m_append_infix(symbol, precedence, associativity, m_make_fixed_overload<Sig, Function>(semantics));
    }

    template <auto Function>
    constexpr auto add_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return add_infix_operator<fw::detail::fn_sig_t<decltype(Function)>, Function>(symbol, precedence, associativity, semantics);
    }

    template <class Sig, class F>
    constexpr auto add_infix_operator(
        std::string_view symbol,
        F function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return add_infix_operator<fw::detail::fn_sig_t<decltype(+function)>, +function>(symbol, precedence, associativity, semantics);
    }

    constexpr auto add_infix_operator(
        std::string_view symbol,
        auto function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return add_infix_operator<fw::detail::fn_sig_t<decltype(+function)>, +function>(symbol, precedence, associativity, semantics);
    }

    template <class Sig, auto Function>
    constexpr auto register_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return add_infix_operator<Sig, Function>(symbol, precedence, associativity, semantics);
    }

    template <auto Function>
    constexpr auto register_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return add_infix_operator<Function>(symbol, precedence, associativity, semantics);
    }

    template <class Sig, class F>
    constexpr auto register_infix_operator(
        std::string_view symbol,
        F function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return add_infix_operator<Sig>(symbol, function, precedence, associativity, semantics);
    }

    template <class Sig, class... Rest, class F>
    constexpr auto add_infix_operator_overloads(
        std::string_view symbol,
        F function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        auto next = add_infix_operator<Sig>(symbol, function, precedence, associativity, semantics);
        if constexpr (sizeof...(Rest) == 0)
        {
            return next;
        }
        else
        {
            return next.template add_infix_operator_overloads<Rest...>(symbol, function, precedence, associativity, semantics);
        }
    }

    template <class... Ts, class F>
    constexpr auto add_infix_operator_for(
        std::string_view symbol,
        F function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return add_infix_operator_overloads<typename binary_signature_for<Ts, decltype(function)>::type...>(
            symbol,
            function,
            precedence,
            associativity,
            semantics);
    }

    constexpr std::optional<MathValue> find_value(std::string_view name) const
    {
        for (std::size_t index = 0; index < VariableCount; ++index)
        {
            if (m_data.m_values[index].name == name)
            {
                return m_data.m_values[index].value;
            }
        }
        return std::nullopt;
    }

    constexpr std::string_view match_infix_symbol(std::string_view text) const
    {
        return m_match_symbol(text, m_data.m_infix_operators);
    }

    constexpr std::string_view match_prefix_symbol(std::string_view text) const
    {
        return m_match_symbol(text, m_data.m_prefix_operators);
    }

    constexpr std::string_view match_postfix_symbol(std::string_view text) const
    {
        return m_match_symbol(text, m_data.m_postfix_operators);
    }

    constexpr const StaticCallableEntry* find_infix_operator(std::string_view symbol) const
    {
        return m_find_entry(symbol, m_data.m_infix_operators);
    }

    constexpr const StaticCallableEntry* find_prefix_operator(std::string_view symbol) const
    {
        return m_find_entry(symbol, m_data.m_prefix_operators);
    }

    constexpr const StaticCallableEntry* find_postfix_operator(std::string_view symbol) const
    {
        return m_find_entry(symbol, m_data.m_postfix_operators);
    }

    constexpr const StaticCallableEntry* find_function(std::string_view name) const
    {
        return m_find_entry(name, m_data.m_functions);
    }

    constexpr CollectedCallable lookup_infix_operator(std::string_view symbol) const
    {
        return m_collect(symbol, m_data.m_infix_operators);
    }

    constexpr CollectedCallable lookup_prefix_operator(std::string_view symbol) const
    {
        return m_collect(symbol, m_data.m_prefix_operators);
    }

    constexpr CollectedCallable lookup_postfix_operator(std::string_view symbol) const
    {
        return m_collect(symbol, m_data.m_postfix_operators);
    }

    constexpr CollectedCallable lookup_function(std::string_view name) const
    {
        return m_collect(name, m_data.m_functions);
    }

    constexpr const EvaluationPolicy& policy() const noexcept
    {
        return m_data.m_policy;
    }

    constexpr auto with_policy(EvaluationPolicy policy) const
    {
        auto next = *this;
        next.m_data.m_policy = policy;
        return next;
    }

private:
    template <class Sig, auto Function>
    static constexpr MathValue m_invoke_fixed(const void*, const MathValue* arguments, std::size_t count)
    {
        using traits = signature_traits<Sig>;
        using result_t = typename traits::result_type;
        static_assert(is_supported_value_type_v<result_t>, "unsupported constexpr callable result type");
        static_assert(signature_types_supported<Sig>, "unsupported constexpr callable argument type");
        if (count != traits::arity)
        {
            throw std::invalid_argument("string_math: constexpr callable argument count mismatch");
        }
        return m_invoke_fixed_callable_impl<traits>(
            Function,
            arguments,
            count,
            std::make_index_sequence<traits::arity>{});
    }

    template <class Sig, auto Function>
    static constexpr CallableOverloadView m_make_fixed_overload(CallableSemantics semantics)
    {
        using traits = signature_traits<Sig>;
        using result_t = typename traits::result_type;
        static_assert(is_supported_value_type_v<result_t>, "unsupported constexpr callable result type");
        static_assert(signature_types_supported<Sig>, "unsupported constexpr callable argument type");
        return CallableOverloadView{
            value_type_of<result_t>::value,
            ArityKind::Fixed,
            traits::arity,
            traits::arity,
            std::span<const ValueType>(k_signature_arg_types<Sig>.data(), k_signature_arg_types<Sig>.size()),
            semantics,
            nullptr,
            &StaticMathContext::template m_invoke_fixed<Sig, Function>,
            nullptr,
            nullptr,
        };
    }

    template <class NextContext>
    constexpr void m_copy_into(NextContext& next) const
    {
        for (std::size_t index = 0; index < VariableCount; ++index)
        {
            next.m_data.m_values[index] = m_data.m_values[index];
        }
        for (std::size_t index = 0; index < InfixCount; ++index)
        {
            next.m_data.m_infix_operators[index] = m_data.m_infix_operators[index];
        }
        for (std::size_t index = 0; index < FunctionCount; ++index)
        {
            next.m_data.m_functions[index] = m_data.m_functions[index];
        }
        for (std::size_t index = 0; index < PrefixCount; ++index)
        {
            next.m_data.m_prefix_operators[index] = m_data.m_prefix_operators[index];
        }
        for (std::size_t index = 0; index < PostfixCount; ++index)
        {
            next.m_data.m_postfix_operators[index] = m_data.m_postfix_operators[index];
        }
        next.m_data.m_policy = m_data.m_policy;
    }

    constexpr auto detail_set_value(std::string_view name, MathValue value) const
    {
        StaticMathContext<VariableCount + 1, InfixCount, FunctionCount, PrefixCount, PostfixCount> next;
        m_copy_into(next);
        next.m_data.m_values[VariableCount] = StaticValueEntry{name, value};
        return next;
    }

    constexpr auto m_append_function(std::string_view name, CallableOverloadView overload) const
    {
        StaticMathContext<VariableCount, InfixCount, FunctionCount + 1, PrefixCount, PostfixCount> next;
        m_copy_into(next);
        next.m_data.m_functions[FunctionCount] = StaticCallableEntry{name, 0, Associativity::Left, overload};
        return next;
    }

    constexpr auto m_append_prefix(std::string_view symbol, int precedence, CallableOverloadView overload) const
    {
        StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount + 1, PostfixCount> next;
        m_copy_into(next);
        next.m_data.m_prefix_operators[PrefixCount] = StaticCallableEntry{symbol, precedence, Associativity::Left, overload};
        return next;
    }

    constexpr auto m_append_postfix(std::string_view symbol, int precedence, CallableOverloadView overload) const
    {
        StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount + 1> next;
        m_copy_into(next);
        next.m_data.m_postfix_operators[PostfixCount] = StaticCallableEntry{symbol, precedence, Associativity::Left, overload};
        return next;
    }

    constexpr auto m_append_infix(
        std::string_view symbol,
        int precedence,
        Associativity associativity,
        CallableOverloadView overload) const
    {
        StaticMathContext<VariableCount, InfixCount + 1, FunctionCount, PrefixCount, PostfixCount> next;
        m_copy_into(next);
        next.m_data.m_infix_operators[InfixCount] = StaticCallableEntry{symbol, precedence, associativity, overload};
        return next;
    }

    template <std::size_t Count>
    static constexpr const StaticCallableEntry* m_find_entry(
        std::string_view name,
        const std::array<StaticCallableEntry, Count>& entries)
    {
        for (std::size_t index = 0; index < Count; ++index)
        {
            if (entries[index].name_or_symbol == name)
            {
                return &entries[index];
            }
        }
        return nullptr;
    }

    template <std::size_t Count>
    static constexpr std::string_view m_match_symbol(std::string_view text, const std::array<StaticCallableEntry, Count>& entries)
    {
        std::string_view best;
        for (std::size_t index = 0; index < Count; ++index)
        {
            const auto symbol = entries[index].name_or_symbol;
            if (text.size() >= symbol.size() && text.substr(0, symbol.size()) == symbol && symbol.size() > best.size())
            {
                best = symbol;
            }
        }
        return best;
    }

    template <std::size_t Count>
    static constexpr CollectedCallable m_collect(std::string_view name, const std::array<StaticCallableEntry, Count>& entries)
    {
        CollectedCallable collected;
        for (std::size_t index = 0; index < Count; ++index)
        {
            if (entries[index].name_or_symbol != name)
            {
                continue;
            }
            if (collected.overloads.empty())
            {
                collected.precedence = entries[index].precedence;
                collected.associativity = entries[index].associativity;
            }
            collected.overloads.push_back(entries[index].overload);
        }
        return collected;
    }

    StaticContextData<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> m_data{};

    template <std::size_t, std::size_t, std::size_t, std::size_t, std::size_t>
    friend class StaticMathContext;
};

#endif // STRING_MATH_HAS_CONSTEXPR_EVALUATION

} // namespace string_math::internal

namespace string_math
{

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
inline constexpr auto MathContext::compile_time()
{
    return internal::StaticMathContext<>{};
}
#endif

} // namespace string_math
