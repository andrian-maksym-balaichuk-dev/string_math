#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include <string_math/internal/context/context.hpp>
#include <string_math/internal/context/context_data.hpp>
#include <string_math/internal/callable/callable.hpp>
#include <string_math/internal/support/fixed_vector.hpp>

namespace string_math::internal
{

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION

template <
    std::size_t VariableCapacity,
    std::size_t InfixCapacity,
    std::size_t FunctionCapacity,
    std::size_t PrefixCapacity,
    std::size_t PostfixCapacity>
class StaticMathContext
{
public:
    static constexpr bool is_constexpr_context = true;
    static constexpr std::size_t max_variables = VariableCapacity;
    static constexpr std::size_t max_infix_operators = InfixCapacity;
    static constexpr std::size_t max_functions = FunctionCapacity;
    static constexpr std::size_t max_prefix_operators = PrefixCapacity;
    static constexpr std::size_t max_postfix_operators = PostfixCapacity;

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

    template <class Invoke, class PolicyInvoke = MathPolicyCallable>
        requires(
            (std::is_same_v<std::decay_t<Invoke>, MathCallable> || std::is_same_v<std::decay_t<Invoke>, RawMathCallable>) &&
            (std::is_same_v<std::decay_t<PolicyInvoke>, MathPolicyCallable> ||
                std::is_same_v<std::decay_t<PolicyInvoke>, RawMathPolicyCallable>))
    constexpr auto add_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        Invoke&& invoke,
        ValueType result_type,
        CallableSemantics semantics = {},
        PolicyInvoke&& policy_invoke = {}) const
    {
        return add_variadic_function(
            name,
            min_arity,
            k_unbounded_arity,
            std::forward<Invoke>(invoke),
            result_type,
            semantics,
            std::forward<PolicyInvoke>(policy_invoke));
    }

    template <class Invoke, class PolicyInvoke = MathPolicyCallable>
        requires(
            (std::is_same_v<std::decay_t<Invoke>, MathCallable> || std::is_same_v<std::decay_t<Invoke>, RawMathCallable>) &&
            (std::is_same_v<std::decay_t<PolicyInvoke>, MathPolicyCallable> ||
                std::is_same_v<std::decay_t<PolicyInvoke>, RawMathPolicyCallable>))
    constexpr auto add_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        Invoke&& invoke,
        ValueType result_type,
        CallableSemantics semantics = {},
        PolicyInvoke&& policy_invoke = {}) const
    {
        const auto invoke_pack = m_make_erased_invoke(std::forward<Invoke>(invoke));
        const auto policy_pack = m_make_erased_policy(std::forward<PolicyInvoke>(policy_invoke));

        return m_append_function(
            name,
            CallableOverloadView{
                result_type,
                ArityKind::Variadic,
                min_arity,
                max_arity,
                {},
                semantics,
                invoke_pack.invoke,
                invoke_pack.raw_invoke,
                policy_pack.policy_invoke,
                policy_pack.raw_policy_invoke,
            });
    }

    template <auto Function>
    constexpr auto add_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        CallableSemantics semantics = {}) const
    {
        return add_variadic_function<Function>(name, min_arity, k_unbounded_arity, semantics);
    }

    template <auto Function>
    constexpr auto add_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        CallableSemantics semantics = {}) const
    {
        return m_append_function(name, m_make_variadic_overload<Function>(min_arity, max_arity, semantics));
    }

    template <class F>
    constexpr auto add_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {}) const
    {
        return m_append_function(name, m_make_variadic_overload<+function>(min_arity, k_unbounded_arity, semantics));
    }

    template <class F>
    constexpr auto add_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        F&& function,
        CallableSemantics semantics = {}) const
    {
        return m_append_function(name, m_make_variadic_overload<+function>(min_arity, max_arity, semantics));
    }

    template <class T, auto Function>
    constexpr auto add_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        CallableSemantics semantics = {}) const
    {
        return add_variadic_function_for<T, Function>(name, min_arity, k_unbounded_arity, semantics);
    }

    template <class T, auto Function>
    constexpr auto add_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        CallableSemantics semantics = {}) const
    {
        return m_append_function(name, m_make_typed_variadic_overload<T, Function>(min_arity, max_arity, semantics));
    }

    template <class T, class F>
    constexpr auto add_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {}) const
    {
        return m_append_function(
            name,
            m_make_typed_variadic_overload<T, +function>(min_arity, k_unbounded_arity, semantics));
    }

    template <class T, class F>
    constexpr auto add_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        F&& function,
        CallableSemantics semantics = {}) const
    {
        return m_append_function(name, m_make_typed_variadic_overload<T, +function>(min_arity, max_arity, semantics));
    }

    constexpr auto register_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        internal::callable_invoke_wrapper invoke,
        ValueType result_type,
        CallableSemantics semantics = {},
        internal::callable_policy_wrapper policy_invoke = {}) const
    {
        return add_variadic_function(name, min_arity, std::move(invoke), result_type, semantics, std::move(policy_invoke));
    }

    constexpr auto register_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        internal::callable_invoke_wrapper invoke,
        ValueType result_type,
        CallableSemantics semantics = {},
        internal::callable_policy_wrapper policy_invoke = {}) const
    {
        return add_variadic_function(name, min_arity, max_arity, std::move(invoke), result_type, semantics, std::move(policy_invoke));
    }

    template <auto Function>
    constexpr auto register_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        CallableSemantics semantics = {}) const
    {
        return add_variadic_function<Function>(name, min_arity, semantics);
    }

    template <auto Function>
    constexpr auto register_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        CallableSemantics semantics = {}) const
    {
        return add_variadic_function<Function>(name, min_arity, max_arity, semantics);
    }

    template <class F>
    constexpr auto register_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {}) const
    {
        return add_variadic_function(name, min_arity, function, semantics);
    }

    template <class F>
    constexpr auto register_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        F&& function,
        CallableSemantics semantics = {}) const
    {
        return add_variadic_function(name, min_arity, max_arity, function, semantics);
    }

    template <class T, auto Function>
    constexpr auto register_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        CallableSemantics semantics = {}) const
    {
        return add_variadic_function_for<T, Function>(name, min_arity, semantics);
    }

    template <class T, auto Function>
    constexpr auto register_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        CallableSemantics semantics = {}) const
    {
        return add_variadic_function_for<T, Function>(name, min_arity, max_arity, semantics);
    }

    template <class T, class F>
    constexpr auto register_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {}) const
    {
        return add_variadic_function_for<T>(name, min_arity, function, semantics);
    }

    template <class T, class F>
    constexpr auto register_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        F&& function,
        CallableSemantics semantics = {}) const
    {
        return add_variadic_function_for<T>(name, min_arity, max_arity, function, semantics);
    }

    template <class Invoke, class PolicyInvoke = MathPolicyCallable>
        requires(
            (std::is_same_v<std::decay_t<Invoke>, MathCallable> || std::is_same_v<std::decay_t<Invoke>, RawMathCallable>) &&
            (std::is_same_v<std::decay_t<PolicyInvoke>, MathPolicyCallable> ||
                std::is_same_v<std::decay_t<PolicyInvoke>, RawMathPolicyCallable>))
    constexpr auto add_function(
        std::string_view name,
        Invoke&& invoke,
        std::size_t min_arity,
        std::size_t max_arity,
        ValueType result_type = ValueType::Double,
        CallableSemantics semantics = {},
        PolicyInvoke&& policy_invoke = {}) const
    {
        const auto invoke_pack = m_make_erased_invoke(std::forward<Invoke>(invoke));
        const auto policy_pack = m_make_erased_policy(std::forward<PolicyInvoke>(policy_invoke));

        return m_append_function(
            name,
            CallableOverloadView{
                result_type,
                max_arity == k_unbounded_arity ? ArityKind::Variadic : ArityKind::Fixed,
                min_arity,
                max_arity,
                {},
                semantics,
                invoke_pack.invoke,
                invoke_pack.raw_invoke,
                policy_pack.policy_invoke,
                policy_pack.raw_policy_invoke,
            });
    }

    template <class Invoke, class PolicyInvoke = MathPolicyCallable>
        requires(
            (std::is_same_v<std::decay_t<Invoke>, MathCallable> || std::is_same_v<std::decay_t<Invoke>, RawMathCallable>) &&
            (std::is_same_v<std::decay_t<PolicyInvoke>, MathPolicyCallable> ||
                std::is_same_v<std::decay_t<PolicyInvoke>, RawMathPolicyCallable>))
    constexpr auto add_prefix_operator(
        std::string_view symbol,
        int precedence,
        Invoke&& invoke,
        ValueType result_type = ValueType::Double,
        CallableSemantics semantics = {},
        PolicyInvoke&& policy_invoke = {}) const
    {
        const auto invoke_pack = m_make_erased_invoke(std::forward<Invoke>(invoke));
        const auto policy_pack = m_make_erased_policy(std::forward<PolicyInvoke>(policy_invoke));

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
                invoke_pack.invoke,
                invoke_pack.raw_invoke,
                policy_pack.policy_invoke,
                policy_pack.raw_policy_invoke,
            });
    }

    template <class Invoke, class PolicyInvoke = MathPolicyCallable>
        requires(
            (std::is_same_v<std::decay_t<Invoke>, MathCallable> || std::is_same_v<std::decay_t<Invoke>, RawMathCallable>) &&
            (std::is_same_v<std::decay_t<PolicyInvoke>, MathPolicyCallable> ||
                std::is_same_v<std::decay_t<PolicyInvoke>, RawMathPolicyCallable>))
    constexpr auto add_postfix_operator(
        std::string_view symbol,
        int precedence,
        Invoke&& invoke,
        ValueType result_type = ValueType::Double,
        CallableSemantics semantics = {},
        PolicyInvoke&& policy_invoke = {}) const
    {
        const auto invoke_pack = m_make_erased_invoke(std::forward<Invoke>(invoke));
        const auto policy_pack = m_make_erased_policy(std::forward<PolicyInvoke>(policy_invoke));

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
                invoke_pack.invoke,
                invoke_pack.raw_invoke,
                policy_pack.policy_invoke,
                policy_pack.raw_policy_invoke,
            });
    }

    template <class Invoke, class PolicyInvoke = MathPolicyCallable>
        requires(
            (std::is_same_v<std::decay_t<Invoke>, MathCallable> || std::is_same_v<std::decay_t<Invoke>, RawMathCallable>) &&
            (std::is_same_v<std::decay_t<PolicyInvoke>, MathPolicyCallable> ||
                std::is_same_v<std::decay_t<PolicyInvoke>, RawMathPolicyCallable>))
    constexpr auto add_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity,
        Invoke&& invoke,
        ValueType result_type = ValueType::Double,
        CallableSemantics semantics = {},
        PolicyInvoke&& policy_invoke = {}) const
    {
        const auto invoke_pack = m_make_erased_invoke(std::forward<Invoke>(invoke));
        const auto policy_pack = m_make_erased_policy(std::forward<PolicyInvoke>(policy_invoke));

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
                invoke_pack.invoke,
                invoke_pack.raw_invoke,
                policy_pack.policy_invoke,
                policy_pack.raw_policy_invoke,
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

    template <class Sig, auto Function, auto PolicyHandler>
    constexpr auto add_infix_operator_with_policy(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return m_append_infix(
            symbol,
            precedence,
            associativity,
            m_make_fixed_overload_with_policy<Sig, Function, PolicyHandler>(semantics));
    }

    template <class Sig, class F, class PolicyF>
    constexpr auto add_infix_operator_with_policy(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return add_infix_operator_with_policy<Sig, +function, +policy_handler>(
            symbol,
            precedence,
            associativity,
            semantics);
    }

    template <class F, class PolicyF>
    constexpr auto add_infix_operator_with_policy(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return add_infix_operator_with_policy<fw::detail::fn_sig_t<decltype(+function)>>(
            symbol,
            function,
            precedence,
            policy_handler,
            associativity,
            semantics);
    }

    template <class Sig, class... Rest, class F, class PolicyF>
    constexpr auto add_infix_operator_with_policy_overloads(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        auto next = add_infix_operator_with_policy<Sig>(
            symbol,
            function,
            precedence,
            policy_handler,
            associativity,
            semantics);
        if constexpr (sizeof...(Rest) == 0)
        {
            return next;
        }
        else
        {
            return next.template add_infix_operator_with_policy_overloads<Rest...>(
                symbol,
                function,
                precedence,
                policy_handler,
                associativity,
                semantics);
        }
    }

    template <class... Ts, class F, class PolicyF>
    constexpr auto add_infix_operator_with_policy_for(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return add_infix_operator_with_policy_overloads<typename binary_signature_for<Ts, decltype(function)>::type...>(
            symbol,
            function,
            precedence,
            policy_handler,
            associativity,
            semantics);
    }

    template <class Sig, auto Function, auto PolicyHandler>
    constexpr auto register_infix_operator_with_policy(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return add_infix_operator_with_policy<Sig, Function, PolicyHandler>(symbol, precedence, associativity, semantics);
    }

    template <class Sig, class F, class PolicyF>
    constexpr auto register_infix_operator_with_policy(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return add_infix_operator_with_policy<Sig>(
            symbol,
            function,
            precedence,
            policy_handler,
            associativity,
            semantics);
    }

    template <class F, class PolicyF>
    constexpr auto register_infix_operator_with_policy(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return add_infix_operator_with_policy(
            symbol,
            function,
            precedence,
            policy_handler,
            associativity,
            semantics);
    }

    constexpr std::optional<MathValue> find_value(std::string_view name) const
    {
        for (std::size_t index = 0; index < m_data.m_value_count; ++index)
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
        return m_match_symbol(text, m_data.m_infix_operators, m_data.m_infix_operator_count);
    }

    constexpr std::string_view match_prefix_symbol(std::string_view text) const
    {
        return m_match_symbol(text, m_data.m_prefix_operators, m_data.m_prefix_operator_count);
    }

    constexpr std::string_view match_postfix_symbol(std::string_view text) const
    {
        return m_match_symbol(text, m_data.m_postfix_operators, m_data.m_postfix_operator_count);
    }

    constexpr const StaticCallableEntry* find_infix_operator(std::string_view symbol) const
    {
        return m_find_entry(symbol, m_data.m_infix_operators, m_data.m_infix_operator_count);
    }

    constexpr const StaticCallableEntry* find_prefix_operator(std::string_view symbol) const
    {
        return m_find_entry(symbol, m_data.m_prefix_operators, m_data.m_prefix_operator_count);
    }

    constexpr const StaticCallableEntry* find_postfix_operator(std::string_view symbol) const
    {
        return m_find_entry(symbol, m_data.m_postfix_operators, m_data.m_postfix_operator_count);
    }

    constexpr const StaticCallableEntry* find_function(std::string_view name) const
    {
        return m_find_entry(name, m_data.m_functions, m_data.m_function_count);
    }

    constexpr CollectedCallable lookup_infix_operator(std::string_view symbol) const
    {
        return m_collect(symbol, m_data.m_infix_operators, m_data.m_infix_operator_count);
    }

    constexpr CollectedCallable lookup_prefix_operator(std::string_view symbol) const
    {
        return m_collect(symbol, m_data.m_prefix_operators, m_data.m_prefix_operator_count);
    }

    constexpr CollectedCallable lookup_postfix_operator(std::string_view symbol) const
    {
        return m_collect(symbol, m_data.m_postfix_operators, m_data.m_postfix_operator_count);
    }

    constexpr CollectedCallable lookup_function(std::string_view name) const
    {
        return m_collect(name, m_data.m_functions, m_data.m_function_count);
    }

    // Constexpr-native lookups: return FixedVector to avoid transient heap allocation.
    // These bypass CollectedCallable's std::vector and are suitable for consteval contexts.

    constexpr FixedVector<CallableOverloadView, k_max_overloads_per_symbol>
        lookup_constexpr_function(std::string_view name) const
    {
        return m_collect_fixed(name, m_data.m_functions, m_data.m_function_count);
    }

    constexpr FixedVector<CallableOverloadView, k_max_overloads_per_symbol>
        lookup_constexpr_infix(std::string_view symbol) const
    {
        return m_collect_fixed(symbol, m_data.m_infix_operators, m_data.m_infix_operator_count);
    }

    constexpr FixedVector<CallableOverloadView, k_max_overloads_per_symbol>
        lookup_constexpr_prefix(std::string_view symbol) const
    {
        return m_collect_fixed(symbol, m_data.m_prefix_operators, m_data.m_prefix_operator_count);
    }

    constexpr FixedVector<CallableOverloadView, k_max_overloads_per_symbol>
        lookup_constexpr_postfix(std::string_view symbol) const
    {
        return m_collect_fixed(symbol, m_data.m_postfix_operators, m_data.m_postfix_operator_count);
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
    using m_generic_invoke_signature = MathValue(MathArgsView);
    using m_generic_policy_signature = Result<MathValue>(MathArgsView, const EvaluationPolicy&, std::string_view);
    using m_raw_invoke_signature = MathValue(MathArgPointer, MathArgCount);
    using m_raw_policy_signature = Result<MathValue>(MathArgPointer, MathArgCount, const EvaluationPolicy&, std::string_view);

    struct m_erased_invoke_pack
    {
        internal::callable_invoke_wrapper invoke{};
        internal::raw_callable_invoke_wrapper raw_invoke{};
    };

    struct m_erased_policy_pack
    {
        internal::callable_policy_wrapper policy_invoke{};
        internal::raw_callable_policy_wrapper raw_policy_invoke{};
    };

    static constexpr void m_require_capacity(bool has_capacity, const char* message)
    {
        if (!has_capacity)
        {
            throw std::out_of_range(message);
        }
    }

    static constexpr auto m_make_erased_invoke(const internal::callable_invoke_wrapper& invoke)
        -> m_erased_invoke_pack
    {
        return {invoke, {}};
    }

    static constexpr auto m_make_erased_invoke(const internal::raw_callable_invoke_wrapper& invoke)
        -> m_erased_invoke_pack
    {
        return {{}, invoke};
    }

    static constexpr auto m_make_erased_policy(const internal::callable_policy_wrapper& policy_invoke)
        -> m_erased_policy_pack
    {
        return {policy_invoke, {}};
    }

    static constexpr auto m_make_erased_policy(const internal::raw_callable_policy_wrapper& policy_invoke)
        -> m_erased_policy_pack
    {
        return {{}, policy_invoke};
    }

    template <class Sig, auto Function>
    static constexpr MathValue m_invoke_fixed(MathArgsView arguments)
    {
        using traits = signature_traits<Sig>;
        using result_t = typename traits::result_type;
        static_assert(is_supported_value_type_v<result_t>, "unsupported constexpr callable result type");
        static_assert(signature_types_supported<Sig>, "unsupported constexpr callable argument type");
        if (arguments.size() != traits::arity)
        {
            throw std::invalid_argument("string_math: constexpr callable argument count mismatch");
        }
        return m_invoke_fixed_callable_impl<traits>(
            Function,
            arguments,
            std::make_index_sequence<traits::arity>{});
    }

    template <auto Function>
    static constexpr MathValue m_invoke_variadic(MathArgsView arguments)
    {
        if consteval
        {
            FixedVector<MathValue, k_max_constexpr_args> args;
            for (std::size_t index = 0; index < arguments.size(); ++index)
            {
                args.push_back(arguments[index]);
            }
            return Function(args.as_span());
        }
        else
        {
            return Function(std::vector<MathValue>(arguments.begin(), arguments.end()));
        }
    }

    template <class T, auto Function>
    static constexpr MathValue m_invoke_typed_variadic(MathArgsView arguments)
    {
        if consteval
        {
            FixedVector<T, k_max_constexpr_args> converted;
            for (std::size_t index = 0; index < arguments.size(); ++index)
            {
                converted.push_back(arguments[index].template cast<T>());
            }
            return MathValue(Function(converted.as_span()));
        }
        else
        {
            std::vector<T> converted;
            converted.reserve(arguments.size());
            for (std::size_t index = 0; index < arguments.size(); ++index)
            {
                converted.push_back(arguments[index].template cast<T>());
            }
            return MathValue(Function(converted));
        }
    }

    template <class Sig, auto PolicyHandler>
    static constexpr Result<MathValue> m_invoke_fixed_policy(
        MathArgsView arguments,
        const EvaluationPolicy& policy,
        std::string_view token)
    {
        using traits = signature_traits<Sig>;
        using result_t = typename traits::result_type;
        static_assert(is_supported_value_type_v<result_t>, "unsupported constexpr callable result type");
        static_assert(signature_types_supported<Sig>, "unsupported constexpr callable argument type");
        return m_invoke_policy_handler_impl<traits>(
            PolicyHandler,
            arguments,
            policy,
            token,
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
            &StaticMathContext::template m_invoke_fixed<Sig, Function>,
            {},
            {},
            {},
        };
    }

    template <auto Function>
    static constexpr CallableOverloadView m_make_variadic_overload(
        std::size_t min_arity,
        std::size_t max_arity,
        CallableSemantics semantics)
    {
        return CallableOverloadView{
            ValueType::Int,
            ArityKind::Variadic,
            min_arity,
            max_arity,
            {},
            semantics,
            &StaticMathContext::template m_invoke_variadic<Function>,
            {},
            {},
            {},
        };
    }

    template <class T, auto Function>
    static constexpr CallableOverloadView m_make_typed_variadic_overload(
        std::size_t min_arity,
        std::size_t max_arity,
        CallableSemantics semantics)
    {
        static_assert(is_supported_value_type_v<T>, "unsupported constexpr variadic helper argument type");
        return CallableOverloadView{
            value_type_of<T>::value,
            ArityKind::Variadic,
            min_arity,
            max_arity,
            std::span<const ValueType>(k_variadic_argument_type<T>.data(), k_variadic_argument_type<T>.size()),
            semantics,
            &StaticMathContext::template m_invoke_typed_variadic<T, Function>,
            {},
            {},
            {},
        };
    }

    template <class Sig, auto Function, auto PolicyHandler>
    static constexpr CallableOverloadView m_make_fixed_overload_with_policy(CallableSemantics semantics)
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
            &StaticMathContext::template m_invoke_fixed<Sig, Function>,
            {},
            &StaticMathContext::template m_invoke_fixed_policy<Sig, PolicyHandler>,
            {},
        };
    }

    template <class T>
    inline static constexpr std::array<ValueType, 1> k_variadic_argument_type{value_type_of<T>::value};

    constexpr auto detail_set_value(std::string_view name, MathValue value) const
    {
        auto next = *this;
        for (std::size_t index = 0; index < next.m_data.m_value_count; ++index)
        {
            if (next.m_data.m_values[index].name == name)
            {
                next.m_data.m_values[index].value = value;
                return next;
            }
        }

        m_require_capacity(
            next.m_data.m_value_count < VariableCapacity,
            "string_math: constexpr context variable capacity exceeded");
        next.m_data.m_values[next.m_data.m_value_count++] = StaticValueEntry{name, value};
        return next;
    }

    constexpr auto m_append_function(std::string_view name, CallableOverloadView overload) const
    {
        auto next = *this;
        m_require_capacity(
            next.m_data.m_function_count < FunctionCapacity,
            "string_math: constexpr context function capacity exceeded");
        next.m_data.m_functions[next.m_data.m_function_count++] = StaticCallableEntry{name, 0, Associativity::Left, overload};
        return next;
    }

    constexpr auto m_append_prefix(std::string_view symbol, int precedence, CallableOverloadView overload) const
    {
        auto next = *this;
        m_require_capacity(
            next.m_data.m_prefix_operator_count < PrefixCapacity,
            "string_math: constexpr context prefix operator capacity exceeded");
        next.m_data.m_prefix_operators[next.m_data.m_prefix_operator_count++] =
            StaticCallableEntry{symbol, precedence, Associativity::Left, overload};
        return next;
    }

    constexpr auto m_append_postfix(std::string_view symbol, int precedence, CallableOverloadView overload) const
    {
        auto next = *this;
        m_require_capacity(
            next.m_data.m_postfix_operator_count < PostfixCapacity,
            "string_math: constexpr context postfix operator capacity exceeded");
        next.m_data.m_postfix_operators[next.m_data.m_postfix_operator_count++] =
            StaticCallableEntry{symbol, precedence, Associativity::Left, overload};
        return next;
    }

    constexpr auto m_append_infix(
        std::string_view symbol,
        int precedence,
        Associativity associativity,
        CallableOverloadView overload) const
    {
        auto next = *this;
        m_require_capacity(
            next.m_data.m_infix_operator_count < InfixCapacity,
            "string_math: constexpr context infix operator capacity exceeded");
        next.m_data.m_infix_operators[next.m_data.m_infix_operator_count++] =
            StaticCallableEntry{symbol, precedence, associativity, overload};
        return next;
    }

    static constexpr const StaticCallableEntry* m_find_entry(
        std::string_view name,
        const auto& entries,
        std::size_t count)
    {
        for (std::size_t index = 0; index < count; ++index)
        {
            if (entries[index].name_or_symbol == name)
            {
                return &entries[index];
            }
        }
        return nullptr;
    }

    static constexpr std::string_view m_match_symbol(std::string_view text, const auto& entries, std::size_t count)
    {
        std::string_view best;
        for (std::size_t index = 0; index < count; ++index)
        {
            const auto symbol = entries[index].name_or_symbol;
            if (text.size() >= symbol.size() && text.substr(0, symbol.size()) == symbol && symbol.size() > best.size())
            {
                best = symbol;
            }
        }
        return best;
    }

    static constexpr CollectedCallable m_collect(std::string_view name, const auto& entries, std::size_t count)
    {
        CollectedCallable collected;
        for (std::size_t index = 0; index < count; ++index)
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

    static constexpr FixedVector<CallableOverloadView, k_max_overloads_per_symbol>
        m_collect_fixed(std::string_view name, const auto& entries, std::size_t count)
    {
        FixedVector<CallableOverloadView, k_max_overloads_per_symbol> result;
        for (std::size_t index = 0; index < count; ++index)
        {
            if (entries[index].name_or_symbol == name)
            {
                result.push_back(entries[index].overload);
            }
        }
        return result;
    }

    StaticContextData<VariableCapacity, InfixCapacity, FunctionCapacity, PrefixCapacity, PostfixCapacity> m_data{};
};

#endif // STRING_MATH_HAS_CONSTEXPR_EVALUATION

} // namespace string_math::internal

namespace string_math
{

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
template <
    std::size_t VariableCapacity,
    std::size_t InfixCapacity,
    std::size_t FunctionCapacity,
    std::size_t PrefixCapacity,
    std::size_t PostfixCapacity>
inline constexpr auto MathContext::compile_time()
{
    return internal::StaticMathContext<
        VariableCapacity,
        InfixCapacity,
        FunctionCapacity,
        PrefixCapacity,
        PostfixCapacity>{};
}
#endif

} // namespace string_math
