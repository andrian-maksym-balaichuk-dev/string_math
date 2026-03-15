#pragma once

#include <memory>
#include <optional>
#include <set>
#include <string_view>
#include <vector>

#include <string_math/internal/callable/callable.hpp>
#include <string_math/internal/value/value.hpp>
#include <string_math/internal/context/context_data.hpp>
#include <string_math/internal/parser/parser.hpp>

namespace string_math
{

class MathContext;
class FrozenMathContext;
class Operation;
class Calculator;
class MathExpr;
MathValue evaluate_operation(const Operation& operation, const MathContext& context);

namespace internal
{

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
template <
    std::size_t VariableCapacity = 100,
    std::size_t InfixCapacity = 100,
    std::size_t FunctionCapacity = 100,
    std::size_t PrefixCapacity = 100,
    std::size_t PostfixCapacity = 100>
class StaticMathContext;
#endif

} // namespace internal

class MathContext
{
public:
    MathContext();
    MathContext(const MathContext& other);
    MathContext& operator=(const MathContext& other);
    MathContext(MathContext&&) noexcept = default;
    MathContext& operator=(MathContext&&) noexcept = default;
    explicit MathContext(const MathContext& parent, bool inherit_only);

    MathContext& set_parent(const MathContext& parent);
    MathContext with_parent(const MathContext& parent) const
    {
        MathContext copy(*this);
        copy.set_parent(parent);
        return copy;
    }

    bool remove_variable(std::string_view name);
    std::optional<MathValue> find_value(std::string_view name) const;
    FrozenMathContext snapshot() const;
    FrozenMathContext freeze() const;
    std::vector<std::string> variable_names() const;

    MathContext& set_policy(EvaluationPolicy policy)
    {
        m_data->m_policy = policy;
        return *this;
    }

    MathContext with_policy(EvaluationPolicy policy) const
    {
        MathContext copy(*this);
        copy.set_policy(policy);
        return copy;
    }

    const EvaluationPolicy& policy() const noexcept
    {
        return m_data->m_policy;
    }

    MathContext with_value(std::string_view name, MathValue value) const
    {
        MathContext copy(*this);
        copy.detail_set_value(name, value);
        return copy;
    }

    template <std::size_t NameSize, class T>
    MathContext with_value(const char (&name)[NameSize], T value) const
    {
        return with_value(std::string_view(name, NameSize - 1), MathValue(value));
    }

    MathContext& set_value(std::string_view name, MathValue value)
    {
        return detail_set_value(name, value);
    }

    template <std::size_t NameSize, class T>
    MathContext& set_value(const char (&name)[NameSize], T value)
    {
        return set_value(std::string_view(name, NameSize - 1), MathValue(value));
    }

    template <class Sig, class F>
    MathContext& add_function(std::string_view name, F&& function, CallableSemantics semantics = {})
    {
        auto& entry = m_data->m_functions[std::string(name)];
        internal::upsert_overload(
            entry.overloads,
            internal::make_fixed_callable_overload<Sig>(std::forward<F>(function), semantics));
        return *this;
    }

    template <class F>
    MathContext& add_function(std::string_view name, F&& function, CallableSemantics semantics = {})
    {
        return add_function<fw::detail::fn_sig_t<F>>(name, std::forward<F>(function), semantics);
    }

    template <class Sig, class F>
    MathContext with_function(std::string_view name, F&& function, CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.template add_function<Sig>(name, std::forward<F>(function), semantics);
        return copy;
    }

    template <class F>
    MathContext with_function(std::string_view name, F&& function, CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.add_function(name, std::forward<F>(function), semantics);
        return copy;
    }

    template <class Sig, class F>
    MathContext& register_function(std::string_view name, F&& function, CallableSemantics semantics = {})
    {
        return add_function<Sig>(name, std::forward<F>(function), semantics);
    }

    template <class F>
    MathContext& register_function(std::string_view name, F&& function, CallableSemantics semantics = {})
    {
        return add_function(name, std::forward<F>(function), semantics);
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function>
    MathContext& add_function(std::string_view name, CallableSemantics semantics = {})
    {
        return add_function<Sig>(name, Function, semantics);
    }

    template <auto Function>
    MathContext& add_function(std::string_view name, CallableSemantics semantics = {})
    {
        return add_function<fw::detail::fn_sig_t<decltype(Function)>, Function>(name, semantics);
    }

    template <class Sig, auto Function>
    MathContext& register_function(std::string_view name, CallableSemantics semantics = {})
    {
        return add_function<Sig, Function>(name, semantics);
    }

    template <auto Function>
    MathContext& register_function(std::string_view name, CallableSemantics semantics = {})
    {
        return add_function<Function>(name, semantics);
    }
#endif

    template <class... WrapperArgs>
    MathContext& add_function(
        std::string_view name,
        fw::function_wrapper<WrapperArgs...> function,
        CallableSemantics semantics = {})
    {
        return m_add_wrapped_function(
            name,
            std::move(function),
            semantics,
            typename fw::function_wrapper<WrapperArgs...>::signatures_type{});
    }

    template <class... WrapperArgs>
    MathContext& add_function(
        std::string_view name,
        fw::move_only_function_wrapper<WrapperArgs...> function,
        CallableSemantics semantics = {})
    {
        return m_add_wrapped_function(
            name,
            std::move(function),
            semantics,
            typename fw::move_only_function_wrapper<WrapperArgs...>::signatures_type{});
    }

    template <class... Sigs, class F>
    MathContext& add_function_overloads(std::string_view name, F&& function, CallableSemantics semantics = {})
    {
        auto shared_function = std::decay_t<F>(std::forward<F>(function));
        (add_function<Sigs>(name, shared_function, semantics), ...);
        return *this;
    }

    template <class... Ts, class F>
    MathContext& add_function_for(std::string_view name, F&& function, CallableSemantics semantics = {})
    {
        return add_function_overloads<typename internal::unary_signature_for<Ts, std::decay_t<F>>::type...>(
            name,
            std::forward<F>(function),
            semantics);
    }

    template <class... Ts, class F>
    MathContext& add_binary_function_for(std::string_view name, F&& function, CallableSemantics semantics = {})
    {
        return add_function_overloads<typename internal::binary_signature_for<Ts, std::decay_t<F>>::type...>(
            name,
            std::forward<F>(function),
            semantics);
    }

    template <class F>
    MathContext& add_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {})
    {
        return add_variadic_function(name, min_arity, internal::k_unbounded_arity, std::forward<F>(function), semantics);
    }

    template <auto Function>
    MathContext& add_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        CallableSemantics semantics = {})
    {
        return add_variadic_function(name, min_arity, internal::k_unbounded_arity, Function, semantics);
    }

    template <class F>
    MathContext& add_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        F&& function,
        CallableSemantics semantics = {})
    {
        auto& entry = m_data->m_functions[std::string(name)];
        internal::upsert_overload(
            entry.overloads,
            internal::make_variadic_callable_overload(min_arity, max_arity, std::forward<F>(function), semantics));
        return *this;
    }

    template <auto Function>
    MathContext& add_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        CallableSemantics semantics = {})
    {
        return add_variadic_function(name, min_arity, max_arity, Function, semantics);
    }

    template <class Invoke, class PolicyInvoke = MathPolicyCallable>
        requires(
            (std::is_same_v<std::decay_t<Invoke>, MathCallable> || std::is_same_v<std::decay_t<Invoke>, RawMathCallable>) &&
            (std::is_same_v<std::decay_t<PolicyInvoke>, MathPolicyCallable> ||
                std::is_same_v<std::decay_t<PolicyInvoke>, RawMathPolicyCallable>))
    MathContext& add_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        Invoke&& invoke,
        ValueType result_type,
        CallableSemantics semantics = {},
        PolicyInvoke&& policy_invoke = {})
    {
        return add_variadic_function(
            name,
            min_arity,
            internal::k_unbounded_arity,
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
    MathContext& add_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        Invoke&& invoke,
        ValueType result_type,
        CallableSemantics semantics = {},
        PolicyInvoke&& policy_invoke = {})
    {
        const auto invoke_pack = m_make_erased_invoke_owner(std::forward<Invoke>(invoke));
        const auto policy_pack = m_make_erased_policy_owner(std::forward<PolicyInvoke>(policy_invoke));

        auto& entry = m_data->m_functions[std::string(name)];
        internal::upsert_overload(
            entry.overloads,
            internal::RuntimeCallableOverload{
                result_type,
                internal::ArityKind::Variadic,
                min_arity,
                max_arity,
                {},
                semantics,
                invoke_pack.invoke_owner,
                invoke_pack.raw_invoke_owner,
                policy_pack.policy_owner,
                policy_pack.raw_policy_owner,
            });
        return *this;
    }

    template <class Invoke, class PolicyInvoke = MathPolicyCallable>
        requires(
            (std::is_same_v<std::decay_t<Invoke>, MathCallable> || std::is_same_v<std::decay_t<Invoke>, RawMathCallable>) &&
            (std::is_same_v<std::decay_t<PolicyInvoke>, MathPolicyCallable> ||
                std::is_same_v<std::decay_t<PolicyInvoke>, RawMathPolicyCallable>))
    MathContext& add_function(
        std::string_view name,
        Invoke&& invoke,
        std::size_t min_arity,
        std::size_t max_arity,
        ValueType result_type = ValueType::Double,
        CallableSemantics semantics = {},
        PolicyInvoke&& policy_invoke = {})
    {
        const auto invoke_pack = m_make_erased_invoke_owner(std::forward<Invoke>(invoke));
        const auto policy_pack = m_make_erased_policy_owner(std::forward<PolicyInvoke>(policy_invoke));

        auto& entry = m_data->m_functions[std::string(name)];
        internal::upsert_overload(
            entry.overloads,
            internal::RuntimeCallableOverload{
                result_type,
                max_arity == internal::k_unbounded_arity ? internal::ArityKind::Variadic : internal::ArityKind::Fixed,
                min_arity,
                max_arity,
                {},
                semantics,
                invoke_pack.invoke_owner,
                invoke_pack.raw_invoke_owner,
                policy_pack.policy_owner,
                policy_pack.raw_policy_owner,
            });
        return *this;
    }

    template <class T, class F>
    MathContext& add_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {})
    {
        return add_variadic_function_for<T>(
            name,
            min_arity,
            internal::k_unbounded_arity,
            std::forward<F>(function),
            semantics);
    }

    template <class T, auto Function>
    MathContext& add_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        CallableSemantics semantics = {})
    {
        return add_variadic_function_for<T>(name, min_arity, internal::k_unbounded_arity, Function, semantics);
    }

    template <class T, auto Function>
    MathContext& add_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        CallableSemantics semantics = {})
    {
        return add_variadic_function_for<T>(name, min_arity, max_arity, Function, semantics);
    }

    template <class T, class F>
    MathContext& add_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        F&& function,
        CallableSemantics semantics = {})
    {
        auto typed_function = std::decay_t<F>(std::forward<F>(function));
        return detail_add_typed_variadic_function<T>(
            name,
            min_arity,
            max_arity,
            [typed_function](const std::vector<T>& arguments) -> MathValue {
                return MathValue(typed_function(arguments));
            },
            semantics);
    }

    template <class F>
    MathContext& register_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {})
    {
        return add_variadic_function(name, min_arity, std::forward<F>(function), semantics);
    }

    template <auto Function>
    MathContext& register_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        CallableSemantics semantics = {})
    {
        return add_variadic_function<Function>(name, min_arity, semantics);
    }

    template <class F>
    MathContext& register_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        F&& function,
        CallableSemantics semantics = {})
    {
        return add_variadic_function(name, min_arity, max_arity, std::forward<F>(function), semantics);
    }

    template <auto Function>
    MathContext& register_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        CallableSemantics semantics = {})
    {
        return add_variadic_function<Function>(name, min_arity, max_arity, semantics);
    }

    MathContext& register_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        internal::callable_invoke_wrapper invoke,
        ValueType result_type,
        CallableSemantics semantics = {},
        internal::callable_policy_wrapper policy_invoke = {})
    {
        return add_variadic_function(name, min_arity, std::move(invoke), result_type, semantics, std::move(policy_invoke));
    }

    MathContext& register_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        internal::callable_invoke_wrapper invoke,
        ValueType result_type,
        CallableSemantics semantics = {},
        internal::callable_policy_wrapper policy_invoke = {})
    {
        return add_variadic_function(name, min_arity, max_arity, std::move(invoke), result_type, semantics, std::move(policy_invoke));
    }

    template <class T, class F>
    MathContext& register_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {})
    {
        return add_variadic_function_for<T>(name, min_arity, std::forward<F>(function), semantics);
    }

    template <class T, auto Function>
    MathContext& register_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        CallableSemantics semantics = {})
    {
        return add_variadic_function_for<T, Function>(name, min_arity, semantics);
    }

    template <class T, class F>
    MathContext& register_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        F&& function,
        CallableSemantics semantics = {})
    {
        return add_variadic_function_for<T>(name, min_arity, max_arity, std::forward<F>(function), semantics);
    }

    template <class T, auto Function>
    MathContext& register_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        CallableSemantics semantics = {})
    {
        return add_variadic_function_for<T, Function>(name, min_arity, max_arity, semantics);
    }

    template <class F>
    MathContext with_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.add_variadic_function(name, min_arity, std::forward<F>(function), semantics);
        return copy;
    }

    template <class F>
    MathContext with_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        F&& function,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.add_variadic_function(name, min_arity, max_arity, std::forward<F>(function), semantics);
        return copy;
    }

    MathContext with_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        internal::callable_invoke_wrapper invoke,
        ValueType result_type,
        CallableSemantics semantics = {},
        internal::callable_policy_wrapper policy_invoke = {}) const
    {
        MathContext copy(*this);
        copy.add_variadic_function(name, min_arity, std::move(invoke), result_type, semantics, std::move(policy_invoke));
        return copy;
    }

    MathContext with_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        internal::callable_invoke_wrapper invoke,
        ValueType result_type,
        CallableSemantics semantics = {},
        internal::callable_policy_wrapper policy_invoke = {}) const
    {
        MathContext copy(*this);
        copy.add_variadic_function(name, min_arity, max_arity, std::move(invoke), result_type, semantics, std::move(policy_invoke));
        return copy;
    }

    template <class T, class F>
    MathContext with_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.template add_variadic_function_for<T>(name, min_arity, std::forward<F>(function), semantics);
        return copy;
    }

    template <class T, class F>
    MathContext with_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        F&& function,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.template add_variadic_function_for<T>(name, min_arity, max_arity, std::forward<F>(function), semantics);
        return copy;
    }

    bool remove_function(std::string_view name);

    template <class Sig, class F>
    MathContext& add_prefix_operator(std::string_view symbol, F&& function, int precedence, CallableSemantics semantics = {})
    {
        auto& entry = m_data->m_prefix_operators[std::string(symbol)];
        entry.precedence = precedence;
        internal::upsert_overload(
            entry.overloads,
            internal::make_fixed_callable_overload<Sig>(std::forward<F>(function), semantics));
        return *this;
    }

    template <class Invoke, class PolicyInvoke = MathPolicyCallable>
        requires(
            (std::is_same_v<std::decay_t<Invoke>, MathCallable> || std::is_same_v<std::decay_t<Invoke>, RawMathCallable>) &&
            (std::is_same_v<std::decay_t<PolicyInvoke>, MathPolicyCallable> ||
                std::is_same_v<std::decay_t<PolicyInvoke>, RawMathPolicyCallable>))
    MathContext& add_prefix_operator(
        std::string_view symbol,
        int precedence,
        Invoke&& invoke,
        ValueType result_type = ValueType::Double,
        CallableSemantics semantics = {},
        PolicyInvoke&& policy_invoke = {})
    {
        const auto invoke_pack = m_make_erased_invoke_owner(std::forward<Invoke>(invoke));
        const auto policy_pack = m_make_erased_policy_owner(std::forward<PolicyInvoke>(policy_invoke));

        auto& entry = m_data->m_prefix_operators[std::string(symbol)];
        entry.precedence = precedence;
        internal::upsert_overload(
            entry.overloads,
            internal::RuntimeCallableOverload{
                result_type,
                internal::ArityKind::Fixed,
                1,
                1,
                {},
                semantics,
                invoke_pack.invoke_owner,
                invoke_pack.raw_invoke_owner,
                policy_pack.policy_owner,
                policy_pack.raw_policy_owner,
            });
        return *this;
    }

    template <class F>
    MathContext& add_prefix_operator(std::string_view symbol, F&& function, int precedence, CallableSemantics semantics = {})
    {
        return add_prefix_operator<fw::detail::fn_sig_t<F>>(symbol, std::forward<F>(function), precedence, semantics);
    }

    template <class Sig, class F>
    MathContext with_prefix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.template add_prefix_operator<Sig>(symbol, std::forward<F>(function), precedence, semantics);
        return copy;
    }

    template <class F>
    MathContext with_prefix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.add_prefix_operator(symbol, std::forward<F>(function), precedence, semantics);
        return copy;
    }

    template <class Sig, class F>
    MathContext& register_prefix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        CallableSemantics semantics = {})
    {
        return add_prefix_operator<Sig>(symbol, std::forward<F>(function), precedence, semantics);
    }

    template <class F>
    MathContext& register_prefix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        CallableSemantics semantics = {})
    {
        return add_prefix_operator(symbol, std::forward<F>(function), precedence, semantics);
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function>
    MathContext& add_prefix_operator(std::string_view symbol, int precedence, CallableSemantics semantics = {})
    {
        return add_prefix_operator<Sig>(symbol, Function, precedence, semantics);
    }

    template <auto Function>
    MathContext& add_prefix_operator(std::string_view symbol, int precedence, CallableSemantics semantics = {})
    {
        return add_prefix_operator<fw::detail::fn_sig_t<decltype(Function)>, Function>(symbol, precedence, semantics);
    }

    template <class Sig, auto Function>
    MathContext& register_prefix_operator(std::string_view symbol, int precedence, CallableSemantics semantics = {})
    {
        return add_prefix_operator<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function>
    MathContext& register_prefix_operator(std::string_view symbol, int precedence, CallableSemantics semantics = {})
    {
        return add_prefix_operator<Function>(symbol, precedence, semantics);
    }
#endif

    template <class... WrapperArgs>
    MathContext& add_prefix_operator(
        std::string_view symbol,
        fw::function_wrapper<WrapperArgs...> function,
        int precedence,
        CallableSemantics semantics = {})
    {
        return m_add_wrapped_prefix(
            symbol,
            std::move(function),
            precedence,
            semantics,
            typename fw::function_wrapper<WrapperArgs...>::signatures_type{});
    }

    template <class... WrapperArgs>
    MathContext& add_prefix_operator(
        std::string_view symbol,
        fw::move_only_function_wrapper<WrapperArgs...> function,
        int precedence,
        CallableSemantics semantics = {})
    {
        return m_add_wrapped_prefix(
            symbol,
            std::move(function),
            precedence,
            semantics,
            typename fw::move_only_function_wrapper<WrapperArgs...>::signatures_type{});
    }

    template <class... Sigs, class F>
    MathContext& add_prefix_operator_overloads(
        std::string_view symbol,
        F&& function,
        int precedence,
        CallableSemantics semantics = {})
    {
        auto shared_function = std::decay_t<F>(std::forward<F>(function));
        (add_prefix_operator<Sigs>(symbol, shared_function, precedence, semantics), ...);
        return *this;
    }

    template <class... Ts, class F>
    MathContext& add_prefix_operator_for(
        std::string_view symbol,
        F&& function,
        int precedence,
        CallableSemantics semantics = {})
    {
        return add_prefix_operator_overloads<typename internal::unary_signature_for<Ts, std::decay_t<F>>::type...>(
            symbol,
            std::forward<F>(function),
            precedence,
            semantics);
    }

    bool remove_prefix_operator(std::string_view symbol);

    template <class Sig, class F>
    MathContext& add_postfix_operator(std::string_view symbol, F&& function, int precedence, CallableSemantics semantics = {})
    {
        auto& entry = m_data->m_postfix_operators[std::string(symbol)];
        entry.precedence = precedence;
        internal::upsert_overload(
            entry.overloads,
            internal::make_fixed_callable_overload<Sig>(std::forward<F>(function), semantics));
        return *this;
    }

    template <class Invoke, class PolicyInvoke = MathPolicyCallable>
        requires(
            (std::is_same_v<std::decay_t<Invoke>, MathCallable> || std::is_same_v<std::decay_t<Invoke>, RawMathCallable>) &&
            (std::is_same_v<std::decay_t<PolicyInvoke>, MathPolicyCallable> ||
                std::is_same_v<std::decay_t<PolicyInvoke>, RawMathPolicyCallable>))
    MathContext& add_postfix_operator(
        std::string_view symbol,
        int precedence,
        Invoke&& invoke,
        ValueType result_type = ValueType::Double,
        CallableSemantics semantics = {},
        PolicyInvoke&& policy_invoke = {})
    {
        const auto invoke_pack = m_make_erased_invoke_owner(std::forward<Invoke>(invoke));
        const auto policy_pack = m_make_erased_policy_owner(std::forward<PolicyInvoke>(policy_invoke));

        auto& entry = m_data->m_postfix_operators[std::string(symbol)];
        entry.precedence = precedence;
        internal::upsert_overload(
            entry.overloads,
            internal::RuntimeCallableOverload{
                result_type,
                internal::ArityKind::Fixed,
                1,
                1,
                {},
                semantics,
                invoke_pack.invoke_owner,
                invoke_pack.raw_invoke_owner,
                policy_pack.policy_owner,
                policy_pack.raw_policy_owner,
            });
        return *this;
    }

    template <class F>
    MathContext& add_postfix_operator(std::string_view symbol, F&& function, int precedence, CallableSemantics semantics = {})
    {
        return add_postfix_operator<fw::detail::fn_sig_t<F>>(symbol, std::forward<F>(function), precedence, semantics);
    }

    template <class Sig, class F>
    MathContext with_postfix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.template add_postfix_operator<Sig>(symbol, std::forward<F>(function), precedence, semantics);
        return copy;
    }

    template <class F>
    MathContext with_postfix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.add_postfix_operator(symbol, std::forward<F>(function), precedence, semantics);
        return copy;
    }

    template <class Sig, class F>
    MathContext& register_postfix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        CallableSemantics semantics = {})
    {
        return add_postfix_operator<Sig>(symbol, std::forward<F>(function), precedence, semantics);
    }

    template <class F>
    MathContext& register_postfix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        CallableSemantics semantics = {})
    {
        return add_postfix_operator(symbol, std::forward<F>(function), precedence, semantics);
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function>
    MathContext& add_postfix_operator(std::string_view symbol, int precedence, CallableSemantics semantics = {})
    {
        return add_postfix_operator<Sig>(symbol, Function, precedence, semantics);
    }

    template <auto Function>
    MathContext& add_postfix_operator(std::string_view symbol, int precedence, CallableSemantics semantics = {})
    {
        return add_postfix_operator<fw::detail::fn_sig_t<decltype(Function)>, Function>(symbol, precedence, semantics);
    }

    template <class Sig, auto Function>
    MathContext& register_postfix_operator(std::string_view symbol, int precedence, CallableSemantics semantics = {})
    {
        return add_postfix_operator<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function>
    MathContext& register_postfix_operator(std::string_view symbol, int precedence, CallableSemantics semantics = {})
    {
        return add_postfix_operator<Function>(symbol, precedence, semantics);
    }
#endif

    template <class... WrapperArgs>
    MathContext& add_postfix_operator(
        std::string_view symbol,
        fw::function_wrapper<WrapperArgs...> function,
        int precedence,
        CallableSemantics semantics = {})
    {
        return m_add_wrapped_postfix(
            symbol,
            std::move(function),
            precedence,
            semantics,
            typename fw::function_wrapper<WrapperArgs...>::signatures_type{});
    }

    template <class... WrapperArgs>
    MathContext& add_postfix_operator(
        std::string_view symbol,
        fw::move_only_function_wrapper<WrapperArgs...> function,
        int precedence,
        CallableSemantics semantics = {})
    {
        return m_add_wrapped_postfix(
            symbol,
            std::move(function),
            precedence,
            semantics,
            typename fw::move_only_function_wrapper<WrapperArgs...>::signatures_type{});
    }

    template <class... Sigs, class F>
    MathContext& add_postfix_operator_overloads(
        std::string_view symbol,
        F&& function,
        int precedence,
        CallableSemantics semantics = {})
    {
        auto shared_function = std::decay_t<F>(std::forward<F>(function));
        (add_postfix_operator<Sigs>(symbol, shared_function, precedence, semantics), ...);
        return *this;
    }

    template <class... Ts, class F>
    MathContext& add_postfix_operator_for(
        std::string_view symbol,
        F&& function,
        int precedence,
        CallableSemantics semantics = {})
    {
        return add_postfix_operator_overloads<typename internal::unary_signature_for<Ts, std::decay_t<F>>::type...>(
            symbol,
            std::forward<F>(function),
            precedence,
            semantics);
    }

    bool remove_postfix_operator(std::string_view symbol);

    template <class Sig, class F>
    MathContext& add_infix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        auto& entry = m_data->m_infix_operators[std::string(symbol)];
        entry.precedence = precedence;
        entry.associativity = associativity;
        internal::upsert_overload(
            entry.overloads,
            internal::make_fixed_callable_overload<Sig>(std::forward<F>(function), semantics));
        return *this;
    }

    template <class Invoke, class PolicyInvoke = MathPolicyCallable>
        requires(
            (std::is_same_v<std::decay_t<Invoke>, MathCallable> || std::is_same_v<std::decay_t<Invoke>, RawMathCallable>) &&
            (std::is_same_v<std::decay_t<PolicyInvoke>, MathPolicyCallable> ||
                std::is_same_v<std::decay_t<PolicyInvoke>, RawMathPolicyCallable>))
    MathContext& add_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity,
        Invoke&& invoke,
        ValueType result_type = ValueType::Double,
        CallableSemantics semantics = {},
        PolicyInvoke&& policy_invoke = {})
    {
        const auto invoke_pack = m_make_erased_invoke_owner(std::forward<Invoke>(invoke));
        const auto policy_pack = m_make_erased_policy_owner(std::forward<PolicyInvoke>(policy_invoke));

        auto& entry = m_data->m_infix_operators[std::string(symbol)];
        entry.precedence = precedence;
        entry.associativity = associativity;
        internal::upsert_overload(
            entry.overloads,
            internal::RuntimeCallableOverload{
                result_type,
                internal::ArityKind::Fixed,
                2,
                2,
                {},
                semantics,
                invoke_pack.invoke_owner,
                invoke_pack.raw_invoke_owner,
                policy_pack.policy_owner,
                policy_pack.raw_policy_owner,
            });
        return *this;
    }

    template <class F>
    MathContext& add_infix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator<fw::detail::fn_sig_t<F>>(
            symbol,
            std::forward<F>(function),
            precedence,
            associativity,
            semantics);
    }

    template <class Sig, class F>
    MathContext with_infix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.template add_infix_operator<Sig>(symbol, std::forward<F>(function), precedence, associativity, semantics);
        return copy;
    }

    template <class F>
    MathContext with_infix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.add_infix_operator(symbol, std::forward<F>(function), precedence, associativity, semantics);
        return copy;
    }

    template <class Sig, class F>
    MathContext& register_infix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator<Sig>(symbol, std::forward<F>(function), precedence, associativity, semantics);
    }

    template <class F>
    MathContext& register_infix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator(symbol, std::forward<F>(function), precedence, associativity, semantics);
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function>
    MathContext& add_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator<Sig>(symbol, Function, precedence, associativity, semantics);
    }

    template <auto Function>
    MathContext& add_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator<fw::detail::fn_sig_t<decltype(Function)>, Function>(symbol, precedence, associativity, semantics);
    }

    template <class Sig, auto Function>
    MathContext& register_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator<Sig, Function>(symbol, precedence, associativity, semantics);
    }

    template <auto Function>
    MathContext& register_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator<Function>(symbol, precedence, associativity, semantics);
    }
#endif

    template <class... WrapperArgs>
    MathContext& add_infix_operator(
        std::string_view symbol,
        fw::function_wrapper<WrapperArgs...> function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return m_add_wrapped_infix(
            symbol,
            std::move(function),
            precedence,
            associativity,
            semantics,
            typename fw::function_wrapper<WrapperArgs...>::signatures_type{});
    }

    template <class... WrapperArgs>
    MathContext& add_infix_operator(
        std::string_view symbol,
        fw::move_only_function_wrapper<WrapperArgs...> function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return m_add_wrapped_infix(
            symbol,
            std::move(function),
            precedence,
            associativity,
            semantics,
            typename fw::move_only_function_wrapper<WrapperArgs...>::signatures_type{});
    }

    template <class... Sigs, class F>
    MathContext& add_infix_operator_overloads(
        std::string_view symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        auto shared_function = std::decay_t<F>(std::forward<F>(function));
        (add_infix_operator<Sigs>(symbol, shared_function, precedence, associativity, semantics), ...);
        return *this;
    }

    template <class... Ts, class F>
    MathContext& add_infix_operator_for(
        std::string_view symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator_overloads<typename internal::binary_signature_for<Ts, std::decay_t<F>>::type...>(
            symbol,
            std::forward<F>(function),
            precedence,
            associativity,
            semantics);
    }

    template <class Sig, class F, class PolicyF>
    MathContext& add_infix_operator_with_policy(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        auto& entry = m_data->m_infix_operators[std::string(symbol)];
        entry.precedence = precedence;
        entry.associativity = associativity;
        internal::upsert_overload(
            entry.overloads,
            internal::make_fixed_callable_overload_with_policy<Sig>(
                std::forward<F>(function),
                std::forward<PolicyF>(policy_handler),
                semantics));
        return *this;
    }

    template <class Sig, auto Function, auto PolicyHandler>
    MathContext& add_infix_operator_with_policy(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator_with_policy<Sig>(
            symbol,
            Function,
            precedence,
            PolicyHandler,
            associativity,
            semantics);
    }

    template <class F, class PolicyF>
    MathContext& add_infix_operator_with_policy(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator_with_policy<fw::detail::fn_sig_t<F>>(
            symbol,
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
    }

    template <auto Function, auto PolicyHandler>
    MathContext& add_infix_operator_with_policy(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator_with_policy<fw::detail::fn_sig_t<decltype(Function)>>(
            symbol,
            Function,
            precedence,
            PolicyHandler,
            associativity,
            semantics);
    }

    template <class... WrapperArgs, class PolicyHandler>
    MathContext& add_infix_operator_with_policy(
        std::string_view symbol,
        fw::function_wrapper<WrapperArgs...> function,
        int precedence,
        PolicyHandler&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return m_add_wrapped_infix_with_policy(
            symbol,
            std::move(function),
            precedence,
            std::forward<PolicyHandler>(policy_handler),
            associativity,
            semantics,
            typename fw::function_wrapper<WrapperArgs...>::signatures_type{});
    }

    template <class... WrapperArgs, class PolicyHandler>
    MathContext& add_infix_operator_with_policy(
        std::string_view symbol,
        fw::move_only_function_wrapper<WrapperArgs...> function,
        int precedence,
        PolicyHandler&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return m_add_wrapped_infix_with_policy(
            symbol,
            std::move(function),
            precedence,
            std::forward<PolicyHandler>(policy_handler),
            associativity,
            semantics,
            typename fw::move_only_function_wrapper<WrapperArgs...>::signatures_type{});
    }

    template <class Sig, class... Rest, class F, class PolicyF>
    MathContext& add_infix_operator_with_policy_overloads(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        auto shared_function = std::decay_t<F>(std::forward<F>(function));
        auto shared_policy = std::decay_t<PolicyF>(std::forward<PolicyF>(policy_handler));
        add_infix_operator_with_policy<Sig>(
            symbol,
            shared_function,
            precedence,
            shared_policy,
            associativity,
            semantics);
        if constexpr (sizeof...(Rest) != 0)
        {
            add_infix_operator_with_policy_overloads<Rest...>(
                symbol,
                shared_function,
                precedence,
                shared_policy,
                associativity,
                semantics);
        }
        return *this;
    }

    template <class... Ts, class F, class PolicyF>
    MathContext& add_infix_operator_with_policy_for(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator_with_policy_overloads<typename internal::binary_signature_for<Ts, std::decay_t<F>>::type...>(
            symbol,
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
    }

    template <class Sig, class F, class PolicyF>
    MathContext& register_infix_operator_with_policy(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator_with_policy<Sig>(
            symbol,
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
    }

    template <class Sig, auto Function, auto PolicyHandler>
    MathContext& register_infix_operator_with_policy(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator_with_policy<Sig, Function, PolicyHandler>(symbol, precedence, associativity, semantics);
    }

    template <class F, class PolicyF>
    MathContext& register_infix_operator_with_policy(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator_with_policy(
            symbol,
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
    }

    template <auto Function, auto PolicyHandler>
    MathContext& register_infix_operator_with_policy(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator_with_policy<Function, PolicyHandler>(symbol, precedence, associativity, semantics);
    }

    template <class Sig, class F, class PolicyF>
    MathContext with_infix_operator_with_policy(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.template add_infix_operator_with_policy<Sig>(
            symbol,
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
        return copy;
    }

    template <class F, class PolicyF>
    MathContext with_infix_operator_with_policy(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.add_infix_operator_with_policy(
            symbol,
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
        return copy;
    }

    bool remove_infix_operator(std::string_view symbol);

    template <class F>
    MathContext& add_literal_parser(std::string_view prefix, std::string_view suffix, F&& parser)
    {
        m_data->m_literal_parsers.push_back(internal::LiteralParserEntry{
            std::string(prefix),
            std::string(suffix),
            fw::function_wrapper<std::optional<MathValue>(std::string_view)>(std::forward<F>(parser)),
        });
        return *this;
    }

    template <class F>
    MathContext with_literal_parser(std::string_view prefix, std::string_view suffix, F&& parser) const
    {
        MathContext copy(*this);
        copy.add_literal_parser(prefix, suffix, std::forward<F>(parser));
        return copy;
    }

    template <class F>
    MathContext& register_literal_parser(std::string_view prefix, std::string_view suffix, F&& parser)
    {
        return add_literal_parser(prefix, suffix, std::forward<F>(parser));
    }

    void clear_literal_parsers();
    std::optional<MathValue> try_parse_literal(std::string_view token) const;

    const internal::PrefixOperatorEntry* find_prefix_operator(std::string_view symbol) const;
    const internal::PostfixOperatorEntry* find_postfix_operator(std::string_view symbol) const;
    const internal::InfixOperatorEntry* find_infix_operator(std::string_view symbol) const;
    const internal::FunctionEntry* find_function(std::string_view name) const;

    internal::CollectedCallable lookup_prefix_operator(std::string_view symbol) const;
    internal::CollectedCallable lookup_postfix_operator(std::string_view symbol) const;
    internal::CollectedCallable lookup_infix_operator(std::string_view symbol) const;
    internal::CollectedCallable lookup_function(std::string_view name) const;

    std::string match_prefix_symbol(std::string_view text) const;
    std::string match_postfix_symbol(std::string_view text) const;
    std::string match_infix_symbol(std::string_view text) const;
    std::vector<std::string> function_names() const;
    std::vector<std::string> prefix_operator_names() const;
    std::vector<std::string> postfix_operator_names() const;
    std::vector<std::string> infix_operator_names() const;
    std::vector<LiteralParserInfo> literal_parsers() const;
    std::optional<FunctionInfo> inspect_function(std::string_view name) const;
    std::optional<OperatorInfo> inspect_prefix_operator(std::string_view symbol) const;
    std::optional<OperatorInfo> inspect_postfix_operator(std::string_view symbol) const;
    std::optional<OperatorInfo> inspect_infix_operator(std::string_view symbol) const;

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <
        std::size_t VariableCapacity = 100,
        std::size_t InfixCapacity = 100,
        std::size_t FunctionCapacity = 100,
        std::size_t PrefixCapacity = 100,
        std::size_t PostfixCapacity = 100>
    static constexpr auto compile_time();
#endif

private:
    enum class SymbolKind
    {
        Prefix,
        Postfix,
        Infix
    };

    std::shared_ptr<internal::RuntimeContextData> m_data;

    MathContext& detail_set_value(std::string_view name, MathValue value);

    struct m_erased_invoke_owner_pack
    {
        std::shared_ptr<internal::callable_invoke_wrapper> invoke_owner;
        std::shared_ptr<internal::raw_callable_invoke_wrapper> raw_invoke_owner;
    };

    struct m_erased_policy_owner_pack
    {
        std::shared_ptr<internal::callable_policy_wrapper> policy_owner;
        std::shared_ptr<internal::raw_callable_policy_wrapper> raw_policy_owner;
    };

    static auto m_make_erased_invoke_owner(const internal::callable_invoke_wrapper& invoke) -> m_erased_invoke_owner_pack
    {
        return {internal::make_invoke_owner(invoke), {}};
    }

    static auto m_make_erased_invoke_owner(const internal::raw_callable_invoke_wrapper& invoke) -> m_erased_invoke_owner_pack
    {
        return {{}, internal::make_raw_invoke_owner(invoke)};
    }

    static auto m_make_erased_policy_owner(const internal::callable_policy_wrapper& policy_invoke)
        -> m_erased_policy_owner_pack
    {
        return {
            policy_invoke ? internal::make_policy_owner(policy_invoke)
                          : std::shared_ptr<internal::callable_policy_wrapper>{},
            {}};
    }

    static auto m_make_erased_policy_owner(const internal::raw_callable_policy_wrapper& policy_invoke)
        -> m_erased_policy_owner_pack
    {
        return {
            {},
            policy_invoke ? internal::make_raw_policy_owner(policy_invoke)
                          : std::shared_ptr<internal::raw_callable_policy_wrapper>{}};
    }

    template <class T, class F>
    MathContext& detail_add_typed_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        std::size_t max_arity,
        F&& function,
        CallableSemantics semantics)
    {
        auto& entry = m_data->m_functions[std::string(name)];
        internal::upsert_overload(
            entry.overloads,
            internal::make_typed_variadic_callable_overload<T>(
                min_arity,
                max_arity,
                std::forward<F>(function),
                semantics));
        return *this;
    }

    template <class EntryMap, class Wrapper, class... Sigs>
    MathContext& m_add_wrapped_fixed_overloads(
        std::string_view name,
        EntryMap internal::RuntimeContextData::* member,
        Wrapper&& function,
        int precedence,
        Associativity associativity,
        CallableSemantics semantics,
        fw::detail::typelist<Sigs...>)
    {
        using wrapper_t = std::decay_t<Wrapper>;
        auto shared_function = std::make_shared<wrapper_t>(std::forward<Wrapper>(function));
        auto& entry = (m_data.get()->*member)[std::string(name)];
        entry.precedence = precedence;
        entry.associativity = associativity;
        (internal::upsert_overload(
             entry.overloads,
             internal::make_fixed_wrapper_overload<Sigs>(shared_function, semantics)),
         ...);
        return *this;
    }

    template <class Wrapper, class... Sigs>
    MathContext& m_add_wrapped_function(
        std::string_view name,
        Wrapper&& function,
        CallableSemantics semantics,
        fw::detail::typelist<Sigs...> signatures)
    {
        return m_add_wrapped_fixed_overloads(
            name,
            &internal::RuntimeContextData::m_functions,
            std::forward<Wrapper>(function),
            0,
            Associativity::Left,
            semantics,
            signatures);
    }

    template <class Wrapper, class... Sigs>
    MathContext& m_add_wrapped_prefix(
        std::string_view symbol,
        Wrapper&& function,
        int precedence,
        CallableSemantics semantics,
        fw::detail::typelist<Sigs...> signatures)
    {
        return m_add_wrapped_fixed_overloads(
            symbol,
            &internal::RuntimeContextData::m_prefix_operators,
            std::forward<Wrapper>(function),
            precedence,
            Associativity::Left,
            semantics,
            signatures);
    }

    template <class Wrapper, class... Sigs>
    MathContext& m_add_wrapped_postfix(
        std::string_view symbol,
        Wrapper&& function,
        int precedence,
        CallableSemantics semantics,
        fw::detail::typelist<Sigs...> signatures)
    {
        return m_add_wrapped_fixed_overloads(
            symbol,
            &internal::RuntimeContextData::m_postfix_operators,
            std::forward<Wrapper>(function),
            precedence,
            Associativity::Left,
            semantics,
            signatures);
    }

    template <class Wrapper, class... Sigs>
    MathContext& m_add_wrapped_infix(
        std::string_view symbol,
        Wrapper&& function,
        int precedence,
        Associativity associativity,
        CallableSemantics semantics,
        fw::detail::typelist<Sigs...> signatures)
    {
        return m_add_wrapped_fixed_overloads(
            symbol,
            &internal::RuntimeContextData::m_infix_operators,
            std::forward<Wrapper>(function),
            precedence,
            associativity,
            semantics,
            signatures);
    }

    template <class Wrapper, class PolicyHandler, class... Sigs>
    MathContext& m_add_wrapped_infix_with_policy(
        std::string_view symbol,
        Wrapper&& function,
        int precedence,
        PolicyHandler&& policy_handler,
        Associativity associativity,
        CallableSemantics semantics,
        fw::detail::typelist<Sigs...>)
    {
        using wrapper_t = std::decay_t<Wrapper>;
        using policy_t = std::decay_t<PolicyHandler>;
        auto shared_function = std::make_shared<wrapper_t>(std::forward<Wrapper>(function));
        auto shared_policy = std::make_shared<policy_t>(std::forward<PolicyHandler>(policy_handler));
        auto& entry = m_data->m_infix_operators[std::string(symbol)];
        entry.precedence = precedence;
        entry.associativity = associativity;
        (internal::upsert_overload(
             entry.overloads,
             internal::make_fixed_wrapper_overload_with_policy<Sigs>(shared_function, shared_policy, semantics)),
         ...);
        return *this;
    }

    template <class EntryMap>
    const typename EntryMap::mapped_type* m_find_entry(std::string_view name, const EntryMap internal::RuntimeContextData::* member) const
    {
        const std::string key(name);
        for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
        {
            const auto& map = current->*member;
            const auto found = map.find(key);
            if (found != map.end())
            {
                return &found->second;
            }
        }
        return nullptr;
    }

    template <class EntryMap>
    internal::CollectedCallable m_lookup_entry(std::string_view name, const EntryMap internal::RuntimeContextData::* member) const
    {
        if (const auto* entry = m_find_entry(name, member); entry != nullptr)
        {
            return entry->collect();
        }
        return {};
    }

    template <class EntryMap>
    std::vector<std::string> m_collect_names(const EntryMap internal::RuntimeContextData::* member) const
    {
        std::set<std::string> names;
        for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
        {
            for (const auto& entry : current->*member)
            {
                names.insert(entry.first);
            }
        }
        return std::vector<std::string>(names.begin(), names.end());
    }

    std::string m_match_symbol(std::string_view text, SymbolKind kind) const;
};

class FrozenMathContext
{
public:
    FrozenMathContext() = default;

    explicit FrozenMathContext(MathContext context)
        : m_context(std::move(context))
    {}

    const MathContext& context() const noexcept
    {
        return m_context;
    }

    operator const MathContext&() const noexcept
    {
        return m_context;
    }

private:
    MathContext m_context;
};

inline MathContext::MathContext()
    : m_data(std::make_shared<internal::RuntimeContextData>())
{}

inline MathContext::MathContext(const MathContext& other)
    : m_data(std::make_shared<internal::RuntimeContextData>(*other.m_data))
{}

inline MathContext& MathContext::operator=(const MathContext& other)
{
    if (this != &other)
    {
        m_data = std::make_shared<internal::RuntimeContextData>(*other.m_data);
    }
    return *this;
}

inline MathContext::MathContext(const MathContext& parent, bool inherit_only)
    : m_data(std::make_shared<internal::RuntimeContextData>())
{
    (void)inherit_only;
    m_data->m_parent = parent.m_data;
}

inline MathContext& MathContext::set_parent(const MathContext& parent)
{
    m_data->m_parent = parent.m_data;
    return *this;
}

inline MathContext& MathContext::detail_set_value(std::string_view name, MathValue value)
{
    m_data->m_values[std::string(name)] = value;
    return *this;
}

inline bool MathContext::remove_variable(std::string_view name)
{
    return m_data->m_values.erase(std::string(name)) != 0;
}

inline std::optional<MathValue> MathContext::find_value(std::string_view name) const
{
    const std::string key(name);
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        const auto found = current->m_values.find(key);
        if (found != current->m_values.end())
        {
            return found->second;
        }
    }
    return std::nullopt;
}

inline FrozenMathContext MathContext::snapshot() const
{
    return FrozenMathContext(*this);
}

inline FrozenMathContext MathContext::freeze() const
{
    return snapshot();
}

inline std::vector<std::string> MathContext::variable_names() const
{
    std::set<std::string> names;
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& entry : current->m_values)
        {
            names.insert(entry.first);
        }
    }
    return std::vector<std::string>(names.begin(), names.end());
}

inline bool MathContext::remove_function(std::string_view name)
{
    return m_data->m_functions.erase(std::string(name)) != 0;
}

inline bool MathContext::remove_prefix_operator(std::string_view symbol)
{
    return m_data->m_prefix_operators.erase(std::string(symbol)) != 0;
}

inline bool MathContext::remove_postfix_operator(std::string_view symbol)
{
    return m_data->m_postfix_operators.erase(std::string(symbol)) != 0;
}

inline bool MathContext::remove_infix_operator(std::string_view symbol)
{
    return m_data->m_infix_operators.erase(std::string(symbol)) != 0;
}

inline void MathContext::clear_literal_parsers()
{
    m_data->m_literal_parsers.clear();
}

inline std::optional<MathValue> MathContext::try_parse_literal(std::string_view token) const
{
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& parser : current->m_literal_parsers)
        {
            if (!parser.prefix.empty() && !internal::starts_with(token, parser.prefix))
            {
                continue;
            }
            if (!parser.suffix.empty() && !internal::ends_with(token, parser.suffix))
            {
                continue;
            }
            if (auto value = parser.parser(token))
            {
                return value;
            }
        }
    }
    return std::nullopt;
}

inline const internal::PrefixOperatorEntry* MathContext::find_prefix_operator(std::string_view symbol) const
{
    return m_find_entry(symbol, &internal::RuntimeContextData::m_prefix_operators);
}

inline const internal::PostfixOperatorEntry* MathContext::find_postfix_operator(std::string_view symbol) const
{
    return m_find_entry(symbol, &internal::RuntimeContextData::m_postfix_operators);
}

inline const internal::InfixOperatorEntry* MathContext::find_infix_operator(std::string_view symbol) const
{
    return m_find_entry(symbol, &internal::RuntimeContextData::m_infix_operators);
}

inline const internal::FunctionEntry* MathContext::find_function(std::string_view name) const
{
    return m_find_entry(name, &internal::RuntimeContextData::m_functions);
}

inline internal::CollectedCallable MathContext::lookup_prefix_operator(std::string_view symbol) const
{
    return m_lookup_entry(symbol, &internal::RuntimeContextData::m_prefix_operators);
}

inline internal::CollectedCallable MathContext::lookup_postfix_operator(std::string_view symbol) const
{
    return m_lookup_entry(symbol, &internal::RuntimeContextData::m_postfix_operators);
}

inline internal::CollectedCallable MathContext::lookup_infix_operator(std::string_view symbol) const
{
    return m_lookup_entry(symbol, &internal::RuntimeContextData::m_infix_operators);
}

inline internal::CollectedCallable MathContext::lookup_function(std::string_view name) const
{
    return m_lookup_entry(name, &internal::RuntimeContextData::m_functions);
}

inline std::string MathContext::match_prefix_symbol(std::string_view text) const
{
    return m_match_symbol(text, SymbolKind::Prefix);
}

inline std::string MathContext::match_postfix_symbol(std::string_view text) const
{
    return m_match_symbol(text, SymbolKind::Postfix);
}

inline std::string MathContext::match_infix_symbol(std::string_view text) const
{
    return m_match_symbol(text, SymbolKind::Infix);
}

inline std::vector<std::string> MathContext::function_names() const
{
    return m_collect_names(&internal::RuntimeContextData::m_functions);
}

inline std::vector<std::string> MathContext::prefix_operator_names() const
{
    return m_collect_names(&internal::RuntimeContextData::m_prefix_operators);
}

inline std::vector<std::string> MathContext::postfix_operator_names() const
{
    return m_collect_names(&internal::RuntimeContextData::m_postfix_operators);
}

inline std::vector<std::string> MathContext::infix_operator_names() const
{
    return m_collect_names(&internal::RuntimeContextData::m_infix_operators);
}

inline std::vector<LiteralParserInfo> MathContext::literal_parsers() const
{
    std::vector<LiteralParserInfo> parsers;
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& parser : current->m_literal_parsers)
        {
            parsers.push_back(LiteralParserInfo{parser.prefix, parser.suffix});
        }
    }
    return parsers;
}

inline std::optional<FunctionInfo> MathContext::inspect_function(std::string_view name) const
{
    const auto* entry = find_function(name);
    if (entry == nullptr)
    {
        return std::nullopt;
    }

    FunctionInfo info;
    info.name = std::string(name);
    for (const auto& overload : entry->overloads)
    {
        info.overloads.push_back(CallableOverloadInfo{
            overload.result_type,
            overload.arity_kind,
            overload.min_arity,
            overload.max_arity,
            overload.arg_types,
            overload.semantics,
        });
    }
    return info;
}

inline std::optional<OperatorInfo> MathContext::inspect_prefix_operator(std::string_view symbol) const
{
    const auto* entry = find_prefix_operator(symbol);
    if (entry == nullptr)
    {
        return std::nullopt;
    }

    OperatorInfo info;
    info.symbol = std::string(symbol);
    info.precedence = entry->precedence;
    for (const auto& overload : entry->overloads)
    {
        info.overloads.push_back(CallableOverloadInfo{
            overload.result_type,
            overload.arity_kind,
            overload.min_arity,
            overload.max_arity,
            overload.arg_types,
            overload.semantics,
        });
    }
    return info;
}

inline std::optional<OperatorInfo> MathContext::inspect_postfix_operator(std::string_view symbol) const
{
    const auto* entry = find_postfix_operator(symbol);
    if (entry == nullptr)
    {
        return std::nullopt;
    }

    OperatorInfo info;
    info.symbol = std::string(symbol);
    info.precedence = entry->precedence;
    for (const auto& overload : entry->overloads)
    {
        info.overloads.push_back(CallableOverloadInfo{
            overload.result_type,
            overload.arity_kind,
            overload.min_arity,
            overload.max_arity,
            overload.arg_types,
            overload.semantics,
        });
    }
    return info;
}

inline std::optional<OperatorInfo> MathContext::inspect_infix_operator(std::string_view symbol) const
{
    const auto* entry = find_infix_operator(symbol);
    if (entry == nullptr)
    {
        return std::nullopt;
    }

    OperatorInfo info;
    info.symbol = std::string(symbol);
    info.precedence = entry->precedence;
    info.associativity = entry->associativity;
    for (const auto& overload : entry->overloads)
    {
        info.overloads.push_back(CallableOverloadInfo{
            overload.result_type,
            overload.arity_kind,
            overload.min_arity,
            overload.max_arity,
            overload.arg_types,
            overload.semantics,
        });
    }
    return info;
}

inline std::string MathContext::m_match_symbol(std::string_view text, SymbolKind kind) const
{
    std::string best;
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        const auto* map = &current->m_prefix_operators;
        if (kind == SymbolKind::Postfix)
        {
            map = &current->m_postfix_operators;
        }
        else if (kind == SymbolKind::Infix)
        {
            map = &current->m_infix_operators;
        }

        for (const auto& entry : *map)
        {
            const auto& symbol = entry.first;
            if (text.size() >= symbol.size() && text.substr(0, symbol.size()) == symbol && symbol.size() > best.size())
            {
                best = symbol;
            }
        }
    }

    return best;
}

inline Result<MathValue> try_parse_literal(std::string_view token, const MathContext& context, SourceSpan span = {})
{
    try
    {
        if (auto custom = context.try_parse_literal(token))
        {
            return *custom;
        }
    }
    catch (const std::exception& error)
    {
        return Error(ErrorKind::Literal, error.what(), span, std::string(token));
    }

    try
    {
        return internal::parse_builtin_literal(token);
    }
    catch (const std::exception& error)
    {
        return Error(ErrorKind::Literal, error.what(), span, std::string(token));
    }
}

inline MathValue parse_literal(std::string_view token, const MathContext& context)
{
    return try_parse_literal(token, context).value();
}

} // namespace string_math
