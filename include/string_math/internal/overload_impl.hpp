#pragma once
// Do not include this file directly. Use <string_math/string_math.hpp> or individual public headers.

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <string_math/internal/arithmetic.hpp>
#include <string_math/internal/type_conversion.hpp>
#include <string_math/value/value.hpp>
#include <string_math/value/policy.hpp>
#include <string_math/semantics/semantics.hpp>

namespace string_math::internal
{

struct UnaryOverload
{
    ValueType result_type{};
    ValueType argument_type{};
    CallableSemantics semantics{};
    std::shared_ptr<const void> storage;
    MathValue (*invoke)(const void*, const MathValue&) = nullptr;
};

struct BinaryOverload
{
    ValueType result_type{};
    ValueType left_type{};
    ValueType right_type{};
    CallableSemantics semantics{};
    std::shared_ptr<const void> storage;
    MathValue (*invoke)(const void*, const MathValue&, const MathValue&) = nullptr;
    std::shared_ptr<const void> policy_storage;
    Result<MathValue> (*policy_invoke)(const void*, const MathValue&, const MathValue&, const EvaluationPolicy&, std::string_view) = nullptr;
};

template <class Sig, class F>
inline UnaryOverload make_unary_overload(F&& function, CallableSemantics semantics = {})
{
    using traits = signature_traits<Sig>;
    static_assert(traits::arity == 1, "unary overload requires one parameter");
    using result_t = typename traits::result_type;
    using argument_t = std::tuple_element_t<0, typename traits::args_tuple>;
    static_assert(is_supported_value_type_v<result_t>, "unsupported result type");
    static_assert(is_supported_value_type_v<argument_t>, "unsupported argument type");

    using wrapper_t = fw::function_wrapper<Sig>;
    auto storage = std::make_shared<wrapper_t>(std::forward<F>(function));

    return UnaryOverload{
        value_type_of<result_t>::value,
        value_type_of<argument_t>::value,
        semantics,
        storage,
        [](const void* raw_storage, const MathValue& argument) {
            const auto& wrapper = *static_cast<const wrapper_t*>(raw_storage);
            return MathValue(wrapper(argument.template cast<argument_t>()));
        },
    };
}

template <class Sig, class F>
inline BinaryOverload make_binary_overload(F&& function, CallableSemantics semantics = {})
{
    using traits = signature_traits<Sig>;
    static_assert(traits::arity == 2, "binary overload requires two parameters");
    using result_t = typename traits::result_type;
    using left_t = std::tuple_element_t<0, typename traits::args_tuple>;
    using right_t = std::tuple_element_t<1, typename traits::args_tuple>;
    static_assert(is_supported_value_type_v<result_t>, "unsupported result type");
    static_assert(is_supported_value_type_v<left_t>, "unsupported left type");
    static_assert(is_supported_value_type_v<right_t>, "unsupported right type");

    using wrapper_t = fw::function_wrapper<Sig>;
    auto storage = std::make_shared<wrapper_t>(std::forward<F>(function));

    return BinaryOverload{
        value_type_of<result_t>::value,
        value_type_of<left_t>::value,
        value_type_of<right_t>::value,
        semantics,
        storage,
        [](const void* raw_storage, const MathValue& left, const MathValue& right) {
            const auto& wrapper = *static_cast<const wrapper_t*>(raw_storage);
            return MathValue(wrapper(left.template cast<left_t>(), right.template cast<right_t>()));
        },
        {},
        nullptr,
    };
}

template <class Sig, class F, class PolicyF>
inline BinaryOverload make_binary_overload_with_policy(
    F&& function,
    PolicyF&& policy_handler,
    CallableSemantics semantics = {})
{
    using traits = signature_traits<Sig>;
    static_assert(traits::arity == 2, "binary overload requires two parameters");
    using result_t = typename traits::result_type;
    using left_t = std::tuple_element_t<0, typename traits::args_tuple>;
    using right_t = std::tuple_element_t<1, typename traits::args_tuple>;
    static_assert(is_supported_value_type_v<result_t>, "unsupported result type");
    static_assert(is_supported_value_type_v<left_t>, "unsupported left type");
    static_assert(is_supported_value_type_v<right_t>, "unsupported right type");

    using wrapper_t = fw::function_wrapper<Sig>;
    using policy_wrapper_t = std::decay_t<PolicyF>;

    auto storage = std::make_shared<wrapper_t>(std::forward<F>(function));
    auto policy_storage = std::make_shared<policy_wrapper_t>(std::forward<PolicyF>(policy_handler));

    return BinaryOverload{
        value_type_of<result_t>::value,
        value_type_of<left_t>::value,
        value_type_of<right_t>::value,
        semantics,
        storage,
        [](const void* raw_storage, const MathValue& left, const MathValue& right) {
            const auto& wrapper = *static_cast<const wrapper_t*>(raw_storage);
            return MathValue(wrapper(left.template cast<left_t>(), right.template cast<right_t>()));
        },
        policy_storage,
        [](const void* raw_policy_storage,
           const MathValue& left,
           const MathValue& right,
           const EvaluationPolicy& policy,
           std::string_view token) {
            const auto& wrapper = *static_cast<const policy_wrapper_t*>(raw_policy_storage);
            return std::invoke(
                wrapper,
                left.template cast<left_t>(),
                right.template cast<right_t>(),
                policy,
                token);
        },
    };
}

template <class Overload>
void upsert_overload(std::vector<Overload>& overloads, Overload overload);

template <>
inline void upsert_overload(std::vector<UnaryOverload>& overloads, UnaryOverload overload)
{
    const auto found = std::find_if(overloads.begin(), overloads.end(), [&](const UnaryOverload& current) {
        return current.argument_type == overload.argument_type && current.result_type == overload.result_type;
    });

    if (found != overloads.end())
    {
        *found = std::move(overload);
        return;
    }

    overloads.push_back(std::move(overload));
}

template <>
inline void upsert_overload(std::vector<BinaryOverload>& overloads, BinaryOverload overload)
{
    const auto found = std::find_if(overloads.begin(), overloads.end(), [&](const BinaryOverload& current) {
        return current.left_type == overload.left_type && current.right_type == overload.right_type &&
            current.result_type == overload.result_type;
    });

    if (found != overloads.end())
    {
        *found = std::move(overload);
        return;
    }

    overloads.push_back(std::move(overload));
}

struct ResolutionError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

inline Result<const UnaryOverload*>
try_resolve_unary_overload(const std::vector<UnaryOverload>& overloads, ValueType argument_type, std::string_view symbol)
{
    const UnaryOverload* best = nullptr;
    int best_cost = std::numeric_limits<int>::max();
    bool ambiguous = false;

    for (const auto& overload : overloads)
    {
        const int cost = conversion_cost(argument_type, overload.argument_type);
        if (cost >= 1'000'000)
        {
            continue;
        }

        if (cost < best_cost)
        {
            best = &overload;
            best_cost = cost;
            ambiguous = false;
        }
        else if (cost == best_cost)
        {
            ambiguous = true;
        }
    }

    if (best == nullptr)
    {
        return Error(
            ErrorKind::OverloadResolution,
            "string_math: no matching unary overload for '" + std::string(symbol) + "'",
            {},
            std::string(symbol));
    }

    if (ambiguous)
    {
        return Error(
            ErrorKind::OverloadResolution,
            "string_math: ambiguous unary overload for '" + std::string(symbol) + "'",
            {},
            std::string(symbol));
    }

    return best;
}

inline const UnaryOverload&
resolve_unary_overload(const std::vector<UnaryOverload>& overloads, ValueType argument_type, std::string_view symbol)
{
    return *try_resolve_unary_overload(overloads, argument_type, symbol).value();
}

inline Result<const BinaryOverload*> try_resolve_binary_overload(
    const std::vector<BinaryOverload>& overloads,
    ValueType left_type,
    ValueType right_type,
    std::string_view symbol,
    PromotionPolicy policy = PromotionPolicy::CppLike)
{
    const BinaryOverload* best = nullptr;
    int best_cost = std::numeric_limits<int>::max();
    bool ambiguous = false;
    const ValueType preferred_target = preferred_binary_target(left_type, right_type);

    for (const auto& overload : overloads)
    {
        const int left_cost = conversion_cost(left_type, overload.left_type);
        const int right_cost = conversion_cost(right_type, overload.right_type);
        if (left_cost >= 1'000'000 || right_cost >= 1'000'000)
        {
            continue;
        }

        int preferred_bonus = overload.left_type == preferred_target && overload.right_type == preferred_target ? -50 : 0;
        if (policy == PromotionPolicy::WidenToFloating && is_floating(overload.left_type) && is_floating(overload.right_type))
        {
            preferred_bonus -= 1'000;
        }
        const int total_cost = left_cost + right_cost + preferred_bonus;
        if (total_cost < best_cost)
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

    if (best == nullptr)
    {
        return Error(
            ErrorKind::OverloadResolution,
            "string_math: no matching binary overload for '" + std::string(symbol) + "'",
            {},
            std::string(symbol));
    }

    if (ambiguous)
    {
        return Error(
            ErrorKind::OverloadResolution,
            "string_math: ambiguous binary overload for '" + std::string(symbol) + "'",
            {},
            std::string(symbol));
    }

    return best;
}

inline const BinaryOverload&
resolve_binary_overload(
    const std::vector<BinaryOverload>& overloads,
    ValueType left_type,
    ValueType right_type,
    std::string_view symbol,
    PromotionPolicy policy = PromotionPolicy::CppLike)
{
    return *try_resolve_binary_overload(overloads, left_type, right_type, symbol, policy).value();
}

template <class T>
Result<MathValue> m_apply_binary_integral_policy(
    BinaryPolicyKind kind,
    T left,
    T right,
    const EvaluationPolicy& policy,
    std::string_view token)
{
    T result{};
    bool overflow = false;

    switch (kind)
    {
    case BinaryPolicyKind::IntegralAddOverflow: overflow = m_add_overflow(left, right, result); break;
    case BinaryPolicyKind::IntegralSubtractOverflow: overflow = m_sub_overflow(left, right, result); break;
    case BinaryPolicyKind::IntegralMultiplyOverflow: overflow = m_mul_overflow(left, right, result); break;
    case BinaryPolicyKind::None:
        return Error(
            ErrorKind::Internal,
            "string_math: missing binary policy for integral overflow handling",
            {},
            std::string(token));
    default: return Error(ErrorKind::Internal, "string_math: unsupported binary policy for overflow handling", {}, std::string(token));
    }

    if (!overflow || policy.overflow == OverflowPolicy::Wrap)
    {
        return MathValue(result);
    }

    if (policy.overflow == OverflowPolicy::Checked)
    {
        return Error(ErrorKind::Evaluation, "string_math: arithmetic overflow", {}, std::string(token));
    }

    long double computed = 0.0L;
    switch (kind)
    {
    case BinaryPolicyKind::IntegralAddOverflow:
        computed = static_cast<long double>(left) + static_cast<long double>(right);
        break;
    case BinaryPolicyKind::IntegralSubtractOverflow:
        computed = static_cast<long double>(left) - static_cast<long double>(right);
        break;
    case BinaryPolicyKind::IntegralMultiplyOverflow:
        computed = static_cast<long double>(left) * static_cast<long double>(right);
        break;
    default: break;
    }

    if (policy.overflow == OverflowPolicy::Saturate)
    {
        return MathValue(m_saturate_cast<T>(computed));
    }

    return MathValue(computed);
}

template <class T>
Result<MathValue> m_apply_binary_policy_for_type(
    const BinaryOverload& overload,
    const MathValue& left,
    const MathValue& right,
    const EvaluationPolicy& policy,
    std::string_view token)
{
    if (overload.policy_invoke != nullptr)
    {
        return overload.policy_invoke(overload.policy_storage.get(), left, right, policy, token);
    }

    if (std::is_integral_v<T> && overload.semantics.has_flag(CallableFlag::OverflowSensitive) &&
        overload.semantics.binary_policy != BinaryPolicyKind::None)
    {
        return m_apply_binary_integral_policy(
            overload.semantics.binary_policy,
            left.template cast<T>(),
            right.template cast<T>(),
            policy,
            token);
    }

    return overload.invoke(overload.storage.get(), left, right);
}

inline Result<MathValue> invoke_unary_overload(const UnaryOverload& overload, const MathValue& argument)
{
    return overload.invoke(overload.storage.get(), argument);
}

inline Result<MathValue> invoke_binary_overload(
    const BinaryOverload& overload,
    const MathValue& left,
    const MathValue& right,
    const EvaluationPolicy& policy,
    std::string_view token)
{
    switch (overload.result_type)
    {
    case ValueType::Short: return m_apply_binary_policy_for_type<short>(overload, left, right, policy, token);
    case ValueType::UnsignedShort:
        return m_apply_binary_policy_for_type<unsigned short>(overload, left, right, policy, token);
    case ValueType::Int: return m_apply_binary_policy_for_type<int>(overload, left, right, policy, token);
    case ValueType::UnsignedInt: return m_apply_binary_policy_for_type<unsigned int>(overload, left, right, policy, token);
    case ValueType::Long: return m_apply_binary_policy_for_type<long>(overload, left, right, policy, token);
    case ValueType::UnsignedLong:
        return m_apply_binary_policy_for_type<unsigned long>(overload, left, right, policy, token);
    case ValueType::LongLong: return m_apply_binary_policy_for_type<long long>(overload, left, right, policy, token);
    case ValueType::UnsignedLongLong:
        return m_apply_binary_policy_for_type<unsigned long long>(overload, left, right, policy, token);
    case ValueType::Float: return m_apply_binary_policy_for_type<float>(overload, left, right, policy, token);
    case ValueType::Double: return m_apply_binary_policy_for_type<double>(overload, left, right, policy, token);
    case ValueType::LongDouble:
        return m_apply_binary_policy_for_type<long double>(overload, left, right, policy, token);
    }

    return Error(ErrorKind::Internal, "string_math: unsupported binary overload result type", {}, std::string(token));
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
