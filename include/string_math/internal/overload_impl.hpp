#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <string_math/internal/catalog.hpp>
#include <string_math/internal/type_conversion.hpp>
#include <string_math/value/value.hpp>

namespace string_math::internal
{

struct RuntimeCallableOverload
{
    ValueType result_type{};
    ArityKind arity_kind{ArityKind::Fixed};
    std::size_t min_arity{0};
    std::size_t max_arity{0};
    std::vector<ValueType> arg_types;
    CallableSemantics semantics{};
    std::shared_ptr<const void> state_owner;
    callable_invoke_fn invoke{nullptr};
    std::shared_ptr<const void> policy_state_owner;
    callable_policy_fn policy_invoke{nullptr};

    CallableOverloadView view() const
    {
        return CallableOverloadView{
            result_type,
            arity_kind,
            min_arity,
            max_arity,
            std::span<const ValueType>(arg_types.data(), arg_types.size()),
            semantics,
            state_owner.get(),
            invoke,
            policy_state_owner.get(),
            policy_invoke,
        };
    }
};

template <class ArgsTuple, std::size_t... Indices>
constexpr auto m_make_signature_arg_type_array(std::index_sequence<Indices...>)
{
    return std::array<ValueType, sizeof...(Indices)>{ value_type_of<std::tuple_element_t<Indices, ArgsTuple>>::value... };
}

template <class Sig>
inline constexpr auto k_signature_arg_types =
    m_make_signature_arg_type_array<typename signature_traits<Sig>::args_tuple>(
        std::make_index_sequence<signature_traits<Sig>::arity>{});

template <class Sig>
inline std::vector<ValueType> make_signature_arg_types_vector()
{
    return std::vector<ValueType>(k_signature_arg_types<Sig>.begin(), k_signature_arg_types<Sig>.end());
}

template <class Sig>
inline constexpr bool signature_types_supported =
    []<std::size_t... Indices>(std::index_sequence<Indices...>) {
        return (is_supported_value_type_v<std::tuple_element_t<Indices, typename signature_traits<Sig>::args_tuple>> && ...);
    }(std::make_index_sequence<signature_traits<Sig>::arity>{});

template <class Traits, class Wrapper, std::size_t... Indices>
constexpr MathValue m_invoke_fixed_callable_impl(
    const Wrapper& wrapper,
    const MathValue* arguments,
    std::size_t count,
    std::index_sequence<Indices...>)
{
    if (count != Traits::arity)
    {
        throw std::invalid_argument("string_math: callable argument count mismatch");
    }

    return MathValue(
        wrapper(arguments[Indices].template cast<std::tuple_element_t<Indices, typename Traits::args_tuple>>()...));
}

template <class Traits, class Handler, std::size_t... Indices>
inline Result<MathValue> m_invoke_policy_handler_impl(
    const Handler& handler,
    const MathValue* arguments,
    std::size_t count,
    const EvaluationPolicy& policy,
    std::string_view token,
    std::index_sequence<Indices...>)
{
    if (count != Traits::arity)
    {
        return Error(ErrorKind::Evaluation, "string_math: callable argument count mismatch", {}, std::string(token));
    }

    if constexpr (std::is_same_v<
                      std::invoke_result_t<
                          Handler,
                          std::tuple_element_t<Indices, typename Traits::args_tuple>...,
                          const EvaluationPolicy&,
                          std::string_view>,
                      Result<MathValue>>)
    {
        return std::invoke(
            handler,
            arguments[Indices].template cast<std::tuple_element_t<Indices, typename Traits::args_tuple>>()...,
            policy,
            token);
    }
    else
    {
        return MathValue(std::invoke(
            handler,
            arguments[Indices].template cast<std::tuple_element_t<Indices, typename Traits::args_tuple>>()...,
            policy,
            token));
    }
}

template <class Sig, class F>
inline RuntimeCallableOverload make_fixed_callable_overload(F&& function, CallableSemantics semantics = {})
{
    using traits = signature_traits<Sig>;
    using result_t = typename traits::result_type;
    static_assert(is_supported_value_type_v<result_t>, "unsupported callable result type");
    static_assert(signature_types_supported<Sig>, "unsupported callable argument type");

    using wrapper_t = fw::function_wrapper<Sig>;
    auto storage = std::make_shared<wrapper_t>(std::forward<F>(function));

    return RuntimeCallableOverload{
        value_type_of<result_t>::value,
        ArityKind::Fixed,
        traits::arity,
        traits::arity,
        make_signature_arg_types_vector<Sig>(),
        semantics,
        storage,
        [](const void* raw_state, const MathValue* arguments, std::size_t count) {
            const auto& wrapper = *static_cast<const wrapper_t*>(raw_state);
            return m_invoke_fixed_callable_impl<traits>(wrapper, arguments, count, std::make_index_sequence<traits::arity>{});
        },
        {},
        nullptr,
    };
}

template <class Sig, class F, class PolicyF>
inline RuntimeCallableOverload make_fixed_callable_overload_with_policy(
    F&& function,
    PolicyF&& policy_handler,
    CallableSemantics semantics = {})
{
    using traits = signature_traits<Sig>;
    using result_t = typename traits::result_type;
    static_assert(is_supported_value_type_v<result_t>, "unsupported callable result type");
    static_assert(signature_types_supported<Sig>, "unsupported callable argument type");

    using wrapper_t = fw::function_wrapper<Sig>;
    using policy_wrapper_t = std::decay_t<PolicyF>;

    auto storage = std::make_shared<wrapper_t>(std::forward<F>(function));
    auto policy_storage = std::make_shared<policy_wrapper_t>(std::forward<PolicyF>(policy_handler));

    return RuntimeCallableOverload{
        value_type_of<result_t>::value,
        ArityKind::Fixed,
        traits::arity,
        traits::arity,
        make_signature_arg_types_vector<Sig>(),
        semantics,
        storage,
        [](const void* raw_state, const MathValue* arguments, std::size_t count) {
            const auto& wrapper = *static_cast<const wrapper_t*>(raw_state);
            return m_invoke_fixed_callable_impl<traits>(wrapper, arguments, count, std::make_index_sequence<traits::arity>{});
        },
        policy_storage,
        [](const void* raw_state,
           const MathValue* arguments,
           std::size_t count,
           const EvaluationPolicy& policy,
           std::string_view token) {
            const auto& handler = *static_cast<const policy_wrapper_t*>(raw_state);
            return m_invoke_policy_handler_impl<traits>(
                handler,
                arguments,
                count,
                policy,
                token,
                std::make_index_sequence<traits::arity>{});
        },
    };
}

template <class F>
inline RuntimeCallableOverload make_variadic_callable_overload(
    std::size_t min_arity,
    std::size_t max_arity,
    F&& function,
    CallableSemantics semantics = {})
{
    using wrapper_t = fw::function_wrapper<MathValue(const std::vector<MathValue>&)>;
    auto storage = std::make_shared<wrapper_t>(std::forward<F>(function));

    return RuntimeCallableOverload{
        ValueType::Int,
        ArityKind::Variadic,
        min_arity,
        max_arity,
        {},
        semantics,
        storage,
        [](const void* raw_state, const MathValue* arguments, std::size_t count) {
            const auto& wrapper = *static_cast<const wrapper_t*>(raw_state);
            return wrapper(std::vector<MathValue>(arguments, arguments + count));
        },
        {},
        nullptr,
    };
}

template <class T, class F>
inline RuntimeCallableOverload make_typed_variadic_callable_overload(
    std::size_t min_arity,
    std::size_t max_arity,
    F&& function,
    CallableSemantics semantics = {})
{
    using wrapper_t = fw::function_wrapper<MathValue(const std::vector<T>&)>;
    auto storage = std::make_shared<wrapper_t>(std::forward<F>(function));

    return RuntimeCallableOverload{
        value_type_of<T>::value,
        ArityKind::Variadic,
        min_arity,
        max_arity,
        {value_type_of<T>::value},
        semantics,
        storage,
        [](const void* raw_state, const MathValue* arguments, std::size_t count) {
            const auto& wrapper = *static_cast<const wrapper_t*>(raw_state);
            std::vector<T> converted;
            converted.reserve(count);
            for (std::size_t index = 0; index < count; ++index)
            {
                converted.push_back(arguments[index].template cast<T>());
            }
            return wrapper(converted);
        },
        {},
        nullptr,
    };
}

inline void upsert_overload(std::vector<RuntimeCallableOverload>& overloads, RuntimeCallableOverload overload)
{
    const auto found = std::find_if(overloads.begin(), overloads.end(), [&](const RuntimeCallableOverload& current) {
        return current.result_type == overload.result_type && current.arity_kind == overload.arity_kind &&
            current.min_arity == overload.min_arity && current.max_arity == overload.max_arity &&
            current.arg_types == overload.arg_types;
    });

    if (found != overloads.end())
    {
        *found = std::move(overload);
        return;
    }

    overloads.push_back(std::move(overload));
}

constexpr bool m_all_integral_types(std::span<const ValueType> types)
{
    return std::all_of(types.begin(), types.end(), [](ValueType type) { return is_integral(type); });
}

constexpr bool m_all_floating_types(std::span<const ValueType> types)
{
    return std::all_of(types.begin(), types.end(), [](ValueType type) { return is_floating(type); });
}

constexpr int m_widen_to_floating_bonus(std::span<const ValueType> source_types, std::span<const ValueType> target_types)
{
    if (target_types.empty() || !m_all_floating_types(target_types))
    {
        return 0;
    }

    int bonus = -1'000;
    if (m_all_integral_types(source_types) &&
        std::all_of(target_types.begin(), target_types.end(), [](ValueType type) { return type == ValueType::Double; }))
    {
        bonus -= 100;
    }
    return bonus;
}

constexpr std::optional<ValueType> m_expected_argument_type(const CallableOverloadView& overload, std::size_t index)
{
    if (overload.arg_types.empty())
    {
        return std::nullopt;
    }
    if (overload.arity_kind == ArityKind::Fixed)
    {
        if (index >= overload.arg_types.size())
        {
            return std::nullopt;
        }
        return overload.arg_types[index];
    }
    if (overload.arg_types.size() == 1)
    {
        return overload.arg_types.front();
    }
    if (index < overload.arg_types.size())
    {
        return overload.arg_types[index];
    }
    return overload.arg_types.back();
}

struct OverloadResolutionMatch
{
    const CallableOverloadView* overload{nullptr};
    bool ambiguous{false};
};

constexpr OverloadResolutionMatch resolve_overload_match(
    std::span<const CallableOverloadView> overloads,
    std::span<const ValueType> argument_types,
    PromotionPolicy policy = PromotionPolicy::CppLike)
{
    const CallableOverloadView* best = nullptr;
    int best_cost = std::numeric_limits<int>::max();
    bool ambiguous = false;

    for (const CallableOverloadView& overload : overloads)
    {
        if (!overload.accepts_arity(argument_types.size()))
        {
            continue;
        }

        int total_cost = 0;
        bool supported = true;
        for (std::size_t index = 0; index < argument_types.size(); ++index)
        {
            const auto expected = m_expected_argument_type(overload, index);
            if (!expected)
            {
                continue;
            }

            const int cost = conversion_cost(argument_types[index], *expected);
            if (cost >= 1'000'000)
            {
                supported = false;
                break;
            }
            total_cost += cost;
        }

        if (!supported)
        {
            continue;
        }

        if (argument_types.size() == 2 && overload.arg_types.size() >= 2)
        {
            const ValueType preferred_target = preferred_binary_target(argument_types[0], argument_types[1]);
            int preferred_bonus =
                overload.arg_types[0] == preferred_target && overload.arg_types[1] == preferred_target ? -50 : 0;
            if (policy == PromotionPolicy::WidenToFloating)
            {
                preferred_bonus += m_widen_to_floating_bonus(argument_types, overload.arg_types);
            }
            total_cost += preferred_bonus;
        }
        else if (policy == PromotionPolicy::WidenToFloating)
        {
            total_cost += m_widen_to_floating_bonus(argument_types, overload.arg_types);
        }

        // Untyped generic trampolines should remain a fallback behind explicit typed overloads.
        if (overload.arg_types.empty())
        {
            total_cost += 10'000;
        }

        if (!best || total_cost < best_cost)
        {
            best = &overload;
            best_cost = total_cost;
            ambiguous = false;
        }
        else if (total_cost == best_cost)
        {
            ambiguous = true;
        }
    }

    return {best, ambiguous};
}

inline Result<CallableOverloadView> try_resolve_overload(
    std::span<const CallableOverloadView> overloads,
    std::span<const ValueType> argument_types,
    std::string_view symbol,
    PromotionPolicy policy = PromotionPolicy::CppLike)
{
    const OverloadResolutionMatch match = resolve_overload_match(overloads, argument_types, policy);

    if (match.overload == nullptr)
    {
        return Error(
            ErrorKind::OverloadResolution,
            "string_math: no matching overload for '" + std::string(symbol) + "'",
            {},
            std::string(symbol));
    }

    if (match.ambiguous)
    {
        return Error(
            ErrorKind::OverloadResolution,
            "string_math: ambiguous overload for '" + std::string(symbol) + "'",
            {},
            std::string(symbol));
    }

    return *match.overload;
}

inline Result<MathValue> invoke_overload(
    const CallableOverloadView& overload,
    const MathValue* arguments,
    std::size_t count,
    const EvaluationPolicy& policy,
    std::string_view token)
{
    if (overload.policy_invoke != nullptr)
    {
        return overload.policy_invoke(overload.policy_state, arguments, count, policy, token);
    }
    return overload.invoke(overload.state, arguments, count);
}

template <class T, class F>
struct unary_signature_for
{
    using result_type = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F&, T>>>;
    static_assert(is_supported_value_type_v<result_type>, "unsupported unary helper result type");
    using type = result_type(T);
};

template <class T, class F>
struct binary_signature_for
{
    using result_type = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F&, T, T>>>;
    static_assert(is_supported_value_type_v<result_type>, "unsupported binary helper result type");
    using type = result_type(T, T);
};

} // namespace string_math::internal
