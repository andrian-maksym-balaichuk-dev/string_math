#pragma once

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <string_math/internal/context/context.hpp>
#include <string_math/internal/context/static_context.hpp>
#include <string_math/internal/expression/expression.hpp>
#include <string_math/internal/support/arithmetic.hpp>

namespace string_math::builtin::default_math
{

namespace precedence
{
inline constexpr int conditional = 1;
inline constexpr int logical_or = 2;
inline constexpr int logical_and = 3;
inline constexpr int comparison = 5;
inline constexpr int additive = 10;
inline constexpr int multiplicative = 20;
inline constexpr int power = 30;
inline constexpr int prefix = 40;
inline constexpr int postfix = 50;
} // namespace precedence

namespace constants
{
inline constexpr long double pi = 3.141592653589793238462643383279502884L;
inline constexpr long double e = 2.718281828459045235360287471352662497L;
inline constexpr long double degrees_to_radians = pi / 180.0L;
inline constexpr long double radians_to_degrees = 180.0L / pi;
} // namespace constants

namespace semantics
{
constexpr CallableSemantics pure()
{
    return CallableSemantics(CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::Foldable);
}

constexpr CallableSemantics overflow_sensitive()
{
    return CallableSemantics(CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::OverflowSensitive | CallableFlag::Foldable);
}

constexpr CallableSemantics logical_short_circuit()
{
    return CallableSemantics(
        CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::Foldable | CallableFlag::ShortCircuitCandidate);
}
} // namespace semantics

namespace detail
{

template <class Context, class = void>
struct is_constexpr_context : std::false_type
{};

template <class Context>
struct is_constexpr_context<Context, std::void_t<decltype(Context::is_constexpr_context)>>
    : std::bool_constant<Context::is_constexpr_context>
{};

template <class Context>
inline constexpr bool is_constexpr_context_v = is_constexpr_context<Context>::value;

using invoke_wrapper = internal::callable_invoke_wrapper;
using policy_wrapper = internal::callable_policy_wrapper;

template <class Invoke>
constexpr invoke_wrapper wrap_invoke(Invoke&& invoke)
{
    using invoke_t = std::decay_t<Invoke>;
    if constexpr (fw::detail::supports_any_signature_in_list<invoke_t, typename invoke_wrapper::signatures_type>::value)
    {
        return invoke_wrapper(std::forward<Invoke>(invoke));
    }
    else if constexpr (std::is_constructible_v<RawMathCallable, Invoke>)
    {
        return make_math_callable(RawMathCallable(std::forward<Invoke>(invoke)));
    }
    else
    {
        static_assert(
            fw::detail::supports_any_signature_in_list<invoke_t, typename invoke_wrapper::signatures_type>::value ||
                std::is_constructible_v<RawMathCallable, Invoke>,
            "string_math: unsupported builtin invoke callable");
    }
}

template <class Policy>
constexpr policy_wrapper wrap_policy(Policy&& policy)
{
    using policy_t = std::decay_t<Policy>;
    if constexpr (fw::detail::supports_any_signature_in_list<policy_t, typename policy_wrapper::signatures_type>::value)
    {
        return policy_wrapper(std::forward<Policy>(policy));
    }
    else if constexpr (std::is_constructible_v<RawMathPolicyCallable, Policy>)
    {
        return make_math_policy_callable(RawMathPolicyCallable(std::forward<Policy>(policy)));
    }
    else
    {
        static_assert(
            fw::detail::supports_any_signature_in_list<policy_t, typename policy_wrapper::signatures_type>::value ||
                std::is_constructible_v<RawMathPolicyCallable, Policy>,
            "string_math: unsupported builtin policy callable");
    }
}

template <class T>
constexpr T wrap_add(T left, T right) noexcept
{
    using unsigned_t = std::make_unsigned_t<T>;
    return static_cast<T>(static_cast<unsigned_t>(left) + static_cast<unsigned_t>(right));
}

template <class T>
constexpr T wrap_subtract(T left, T right) noexcept
{
    using unsigned_t = std::make_unsigned_t<T>;
    return static_cast<T>(static_cast<unsigned_t>(left) - static_cast<unsigned_t>(right));
}

template <class T>
constexpr T wrap_multiply(T left, T right) noexcept
{
    using unsigned_t = std::make_unsigned_t<T>;
    return static_cast<T>(static_cast<unsigned_t>(left) * static_cast<unsigned_t>(right));
}

template <class T>
constexpr Result<MathValue> add_policy(T left, T right, const EvaluationPolicy& policy, std::string_view token)
{
    if constexpr (!std::is_integral_v<T>)
    {
        return MathValue(left + right);
    }
    else
    {
        if (policy.promotion == PromotionPolicy::WidenToFloating)
        {
            return MathValue(static_cast<double>(left) + static_cast<double>(right));
        }

        T output{};
        const bool overflow = internal::m_add_overflow(left, right, output);
        if (!overflow)
        {
            return MathValue(output);
        }

        switch (policy.overflow)
        {
        case OverflowPolicy::Wrap:
            return MathValue(wrap_add(left, right));
        case OverflowPolicy::Saturate:
            return MathValue(internal::m_saturate_cast<T>(static_cast<long double>(left) + static_cast<long double>(right)));
        case OverflowPolicy::Promote:
            return MathValue(static_cast<long double>(left) + static_cast<long double>(right));
        case OverflowPolicy::Checked:
            return Error(ErrorKind::Evaluation, "string_math: integral addition overflow", {}, std::string(token));
        }

        return MathValue(output);
    }
}

template <class T>
constexpr Result<MathValue> subtract_policy(T left, T right, const EvaluationPolicy& policy, std::string_view token)
{
    if constexpr (!std::is_integral_v<T>)
    {
        return MathValue(left - right);
    }
    else
    {
        if (policy.promotion == PromotionPolicy::WidenToFloating)
        {
            return MathValue(static_cast<double>(left) - static_cast<double>(right));
        }

        T output{};
        const bool overflow = internal::m_sub_overflow(left, right, output);
        if (!overflow)
        {
            return MathValue(output);
        }

        switch (policy.overflow)
        {
        case OverflowPolicy::Wrap:
            return MathValue(wrap_subtract(left, right));
        case OverflowPolicy::Saturate:
            return MathValue(internal::m_saturate_cast<T>(static_cast<long double>(left) - static_cast<long double>(right)));
        case OverflowPolicy::Promote:
            return MathValue(static_cast<long double>(left) - static_cast<long double>(right));
        case OverflowPolicy::Checked:
            return Error(ErrorKind::Evaluation, "string_math: integral subtraction overflow", {}, std::string(token));
        }

        return MathValue(output);
    }
}

template <class T>
constexpr Result<MathValue> multiply_policy(T left, T right, const EvaluationPolicy& policy, std::string_view token)
{
    if constexpr (!std::is_integral_v<T>)
    {
        return MathValue(left * right);
    }
    else
    {
        if (policy.promotion == PromotionPolicy::WidenToFloating)
        {
            return MathValue(static_cast<double>(left) * static_cast<double>(right));
        }

        T output{};
        const bool overflow = internal::m_mul_overflow(left, right, output);
        if (!overflow)
        {
            return MathValue(output);
        }

        switch (policy.overflow)
        {
        case OverflowPolicy::Wrap:
            return MathValue(wrap_multiply(left, right));
        case OverflowPolicy::Saturate:
            return MathValue(internal::m_saturate_cast<T>(static_cast<long double>(left) * static_cast<long double>(right)));
        case OverflowPolicy::Promote:
            return MathValue(static_cast<long double>(left) * static_cast<long double>(right));
        case OverflowPolicy::Checked:
            return Error(ErrorKind::Evaluation, "string_math: integral multiplication overflow", {}, std::string(token));
        }

        return MathValue(output);
    }
}

// Primitive helpers shared across builtin callables.
constexpr bool truthy(const MathValue& value) noexcept
{
    return value.visit([](const auto& current) { return current != 0; });
}

namespace prefix_ops
{

constexpr MathValue identity(MathArgsView arguments)
{
    return arguments[0];
}

constexpr MathValue negate(MathArgsView arguments)
{
    return arguments[0].visit([](const auto& current) { return MathValue(-current); });
}

constexpr MathValue logical_not(MathArgsView arguments)
{
    return MathValue(truthy(arguments[0]) ? 0 : 1);
}

inline MathValue abs(MathArgsView arguments)
{
    return arguments[0].visit([](const auto& current) -> MathValue {
        using value_t = std::decay_t<decltype(current)>;
        if constexpr (std::is_unsigned_v<value_t>)
        {
            return MathValue(current);
        }
        else
        {
            return MathValue(std::abs(current));
        }
    });
}

inline MathValue sqrt(MathArgsView arguments)
{
    return MathValue(std::sqrt(arguments[0].to_double()));
}

} // namespace prefix_ops

namespace infix_ops
{

constexpr MathValue add(MathArgsView arguments)
{
    return arguments[0] + arguments[1];
}

constexpr MathValue subtract(MathArgsView arguments)
{
    return arguments[0] - arguments[1];
}

constexpr MathValue multiply(MathArgsView arguments)
{
    return arguments[0] * arguments[1];
}

constexpr MathValue divide(MathArgsView arguments)
{
    if (arguments[1] == 0)
    {
        throw std::domain_error("string_math: division by zero");
    }
    return arguments[0] / arguments[1];
}

constexpr MathValue modulo(MathArgsView arguments)
{
    if (arguments[1] == 0)
    {
        throw std::domain_error("string_math: modulo by zero");
    }
    return arguments[0] % arguments[1];
}

inline MathValue power(MathArgsView arguments)
{
    return MathValue(std::pow(arguments[0].to_double(), arguments[1].to_double()));
}

inline MathValue logarithm(MathArgsView arguments)
{
    return MathValue(std::log(arguments[1].to_double()) / std::log(arguments[0].to_double()));
}

constexpr MathValue equal(MathArgsView arguments)
{
    return MathValue(arguments[0] == arguments[1] ? 1 : 0);
}

constexpr MathValue not_equal(MathArgsView arguments)
{
    return MathValue(arguments[0] != arguments[1] ? 1 : 0);
}

constexpr MathValue less(MathArgsView arguments)
{
    return MathValue(arguments[0] < arguments[1] ? 1 : 0);
}

constexpr MathValue less_equal(MathArgsView arguments)
{
    return MathValue(arguments[0] <= arguments[1] ? 1 : 0);
}

constexpr MathValue greater(MathArgsView arguments)
{
    return MathValue(arguments[0] > arguments[1] ? 1 : 0);
}

constexpr MathValue greater_equal(MathArgsView arguments)
{
    return MathValue(arguments[0] >= arguments[1] ? 1 : 0);
}

constexpr MathValue logical_and(MathArgsView arguments)
{
    return MathValue(truthy(arguments[0]) && truthy(arguments[1]) ? 1 : 0);
}

constexpr MathValue logical_or(MathArgsView arguments)
{
    return MathValue(truthy(arguments[0]) || truthy(arguments[1]) ? 1 : 0);
}

constexpr MathValue maximum(MathArgsView arguments)
{
    return arguments[0] > arguments[1] ? arguments[0] : arguments[1];
}

constexpr MathValue minimum(MathArgsView arguments)
{
    return arguments[0] < arguments[1] ? arguments[0] : arguments[1];
}

} // namespace infix_ops

namespace postfix_ops
{

inline MathValue factorial(MathArgsView arguments)
{
    return arguments[0].visit([](const auto& current) -> MathValue {
        using value_t = std::decay_t<decltype(current)>;
        if constexpr (std::is_floating_point_v<value_t>)
        {
            return MathValue(internal::factorial_value(current));
        }
        else
        {
            return MathValue(static_cast<value_t>(internal::factorial_value(static_cast<long double>(current))));
        }
    });
}

} // namespace postfix_ops

namespace function_ops
{

inline MathValue sin(MathArgsView arguments)
{
    return MathValue(std::sin(arguments[0].to_double()));
}

inline MathValue cos(MathArgsView arguments)
{
    return MathValue(std::cos(arguments[0].to_double()));
}

inline MathValue tan(MathArgsView arguments)
{
    return MathValue(std::tan(arguments[0].to_double()));
}

inline MathValue asin(MathArgsView arguments)
{
    return MathValue(std::asin(arguments[0].to_double()));
}

inline MathValue acos(MathArgsView arguments)
{
    return MathValue(std::acos(arguments[0].to_double()));
}

inline MathValue atan(MathArgsView arguments)
{
    return MathValue(std::atan(arguments[0].to_double()));
}

inline MathValue sqrt(MathArgsView arguments)
{
    return MathValue(std::sqrt(arguments[0].to_double()));
}

inline MathValue ceil(MathArgsView arguments)
{
    return MathValue(std::ceil(arguments[0].to_double()));
}

inline MathValue floor(MathArgsView arguments)
{
    return MathValue(std::floor(arguments[0].to_double()));
}

inline MathValue round(MathArgsView arguments)
{
    return MathValue(std::round(arguments[0].to_double()));
}

inline MathValue exp(MathArgsView arguments)
{
    return MathValue(std::exp(arguments[0].to_double()));
}

inline MathValue abs(MathArgsView arguments)
{
    return prefix_ops::abs(arguments);
}

inline MathValue radians(MathArgsView arguments)
{
    return MathValue(arguments[0].to_double() * static_cast<double>(constants::degrees_to_radians));
}

inline MathValue degrees(MathArgsView arguments)
{
    return MathValue(arguments[0].to_double() * static_cast<double>(constants::radians_to_degrees));
}

} // namespace function_ops

namespace variadic_ops
{

constexpr MathValue maximum(MathArgsView arguments)
{
    MathValue current = arguments[0];
    for (std::size_t index = 1; index < arguments.size(); ++index)
    {
        if (arguments[index] > current)
        {
            current = arguments[index];
        }
    }
    return current;
}

constexpr MathValue minimum(MathArgsView arguments)
{
    MathValue current = arguments[0];
    for (std::size_t index = 1; index < arguments.size(); ++index)
    {
        if (arguments[index] < current)
        {
            current = arguments[index];
        }
    }
    return current;
}

} // namespace variadic_ops

namespace policy_ops
{

struct add_fn
{
    template <class T>
    constexpr Result<MathValue> operator()(T left, T right, const EvaluationPolicy& policy, std::string_view token) const
    {
        return add_policy(left, right, policy, token);
    }
};

struct subtract_fn
{
    template <class T>
    constexpr Result<MathValue> operator()(T left, T right, const EvaluationPolicy& policy, std::string_view token) const
    {
        return subtract_policy(left, right, policy, token);
    }
};

struct multiply_fn
{
    template <class T>
    constexpr Result<MathValue> operator()(T left, T right, const EvaluationPolicy& policy, std::string_view token) const
    {
        return multiply_policy(left, right, policy, token);
    }
};

template <class Handler>
constexpr Result<MathValue> dispatch_binary(
    MathArgsView arguments,
    const EvaluationPolicy& policy,
    std::string_view token,
    Handler handler)
{
    if (arguments.size() != 2)
    {
        return Error(ErrorKind::Evaluation, "string_math: callable argument count mismatch", {}, std::string(token));
    }

    switch (internal::preferred_binary_target(arguments[0].type(), arguments[1].type()))
    {
    case ValueType::Short:
        return handler(arguments[0].cast<short>(), arguments[1].cast<short>(), policy, token);
    case ValueType::UnsignedShort:
        return handler(arguments[0].cast<unsigned short>(), arguments[1].cast<unsigned short>(), policy, token);
    case ValueType::Int:
        return handler(arguments[0].cast<int>(), arguments[1].cast<int>(), policy, token);
    case ValueType::UnsignedInt:
        return handler(arguments[0].cast<unsigned int>(), arguments[1].cast<unsigned int>(), policy, token);
    case ValueType::Long:
        return handler(arguments[0].cast<long>(), arguments[1].cast<long>(), policy, token);
    case ValueType::UnsignedLong:
        return handler(arguments[0].cast<unsigned long>(), arguments[1].cast<unsigned long>(), policy, token);
    case ValueType::LongLong:
        return handler(arguments[0].cast<long long>(), arguments[1].cast<long long>(), policy, token);
    case ValueType::UnsignedLongLong:
        return handler(arguments[0].cast<unsigned long long>(), arguments[1].cast<unsigned long long>(), policy, token);
    case ValueType::Float:
        return handler(arguments[0].cast<float>(), arguments[1].cast<float>(), policy, token);
    case ValueType::Double:
        return handler(arguments[0].cast<double>(), arguments[1].cast<double>(), policy, token);
    case ValueType::LongDouble:
        return handler(arguments[0].cast<long double>(), arguments[1].cast<long double>(), policy, token);
    }

    return Error(ErrorKind::Evaluation, "string_math: unsupported builtin binary target", {}, std::string(token));
}

constexpr Result<MathValue> add(
    MathArgsView arguments,
    const EvaluationPolicy& policy,
    std::string_view token)
{
    return dispatch_binary(arguments, policy, token, add_fn{});
}

constexpr Result<MathValue> subtract(
    MathArgsView arguments,
    const EvaluationPolicy& policy,
    std::string_view token)
{
    return dispatch_binary(arguments, policy, token, subtract_fn{});
}

constexpr Result<MathValue> multiply(
    MathArgsView arguments,
    const EvaluationPolicy& policy,
    std::string_view token)
{
    return dispatch_binary(arguments, policy, token, multiply_fn{});
}

} // namespace policy_ops

template <class Context, class Registration>
constexpr auto apply_registration(Context context, Registration&& registration)
{
    if constexpr (is_constexpr_context_v<Context>)
    {
        return registration(std::move(context));
    }
    else
    {
        registration(context);
        return context;
    }
}

template <class Context>
constexpr auto add_constant(Context context, std::string_view name, MathValue value)
{
    return apply_registration(std::move(context), [=](auto&& ctx) { return ctx.set_value(name, value); });
}

template <class Context, class Invoke>
constexpr auto add_infix_builtin(
    Context context,
    std::string_view symbol,
    int precedence_value,
    Associativity associativity,
    Invoke&& invoke,
    ValueType result_type = ValueType::Double,
    CallableSemantics builtin_semantics = semantics::pure(),
    policy_wrapper policy = {})
{
    auto wrapped_invoke = wrap_invoke(std::forward<Invoke>(invoke));
    auto wrapped_policy = std::move(policy);
    return apply_registration(std::move(context), [=](auto&& ctx) mutable {
        return ctx.add_infix_operator(
            symbol,
            precedence_value,
            associativity,
            std::move(wrapped_invoke),
            result_type,
            builtin_semantics,
            std::move(wrapped_policy));
    });
}

template <class Context, class Invoke>
constexpr auto add_prefix_builtin(
    Context context,
    std::string_view symbol,
    int precedence_value,
    Invoke&& invoke,
    ValueType result_type = ValueType::Double,
    CallableSemantics builtin_semantics = semantics::pure(),
    policy_wrapper policy = {})
{
    auto wrapped_invoke = wrap_invoke(std::forward<Invoke>(invoke));
    auto wrapped_policy = std::move(policy);
    return apply_registration(std::move(context), [=](auto&& ctx) mutable {
        return ctx.add_prefix_operator(
            symbol,
            precedence_value,
            std::move(wrapped_invoke),
            result_type,
            builtin_semantics,
            std::move(wrapped_policy));
    });
}

template <class Context, class Invoke>
constexpr auto add_postfix_builtin(
    Context context,
    std::string_view symbol,
    int precedence_value,
    Invoke&& invoke,
    ValueType result_type = ValueType::Double,
    CallableSemantics builtin_semantics = semantics::pure(),
    policy_wrapper policy = {})
{
    auto wrapped_invoke = wrap_invoke(std::forward<Invoke>(invoke));
    auto wrapped_policy = std::move(policy);
    return apply_registration(std::move(context), [=](auto&& ctx) mutable {
        return ctx.add_postfix_operator(
            symbol,
            precedence_value,
            std::move(wrapped_invoke),
            result_type,
            builtin_semantics,
            std::move(wrapped_policy));
    });
}

template <class Context, class Invoke>
constexpr auto add_function_builtin(
    Context context,
    std::string_view name,
    Invoke&& invoke,
    std::size_t min_arity,
    std::size_t max_arity,
    ValueType result_type = ValueType::Double,
    CallableSemantics builtin_semantics = semantics::pure(),
    policy_wrapper policy = {})
{
    auto wrapped_invoke = wrap_invoke(std::forward<Invoke>(invoke));
    auto wrapped_policy = std::move(policy);
    return apply_registration(std::move(context), [=](auto&& ctx) mutable {
        return ctx.add_function(
            name,
            std::move(wrapped_invoke),
            min_arity,
            max_arity,
            result_type,
            builtin_semantics,
            std::move(wrapped_policy));
    });
}

template <class Context, class Invoke>
constexpr auto add_unary_function_builtin(
    Context context,
    std::string_view name,
    Invoke&& invoke,
    ValueType result_type = ValueType::Double,
    CallableSemantics builtin_semantics = semantics::pure(),
    policy_wrapper policy = {})
{
    return add_function_builtin(
        std::move(context),
        name,
        std::forward<Invoke>(invoke),
        1,
        1,
        result_type,
        builtin_semantics,
        std::move(policy));
}

template <class Context>
constexpr auto register_arithmetic_infix(Context context)
{
    auto current = std::move(context);
    current = add_infix_builtin(
        std::move(current),
        "+",
        precedence::additive,
        Associativity::Left,
        infix_ops::add,
        ValueType::Double,
        semantics::overflow_sensitive(),
        wrap_policy(policy_ops::add));
    current = add_infix_builtin(
        std::move(current),
        "-",
        precedence::additive,
        Associativity::Left,
        infix_ops::subtract,
        ValueType::Double,
        semantics::overflow_sensitive(),
        wrap_policy(policy_ops::subtract));
    current = add_infix_builtin(
        std::move(current),
        "*",
        precedence::multiplicative,
        Associativity::Left,
        infix_ops::multiply,
        ValueType::Double,
        semantics::overflow_sensitive(),
        wrap_policy(policy_ops::multiply));
    current = add_infix_builtin(std::move(current), "/", precedence::multiplicative, Associativity::Left, infix_ops::divide);
    current = add_infix_builtin(std::move(current), "%", precedence::multiplicative, Associativity::Left, infix_ops::modulo);
    current = add_infix_builtin(std::move(current), "^", precedence::power, Associativity::Right, infix_ops::power);
    current = add_infix_builtin(std::move(current), "log", precedence::power, Associativity::Left, infix_ops::logarithm);
    current = add_infix_builtin(std::move(current), "max", precedence::power, Associativity::Left, infix_ops::maximum);
    current = add_infix_builtin(std::move(current), "min", precedence::power, Associativity::Left, infix_ops::minimum);
    return current;
}

template <class Context>
constexpr auto register_comparison_infix(Context context)
{
    auto current = std::move(context);
    current = add_infix_builtin(std::move(current), "==", precedence::comparison, Associativity::Left, infix_ops::equal, ValueType::Int);
    current = add_infix_builtin(std::move(current), "!=", precedence::comparison, Associativity::Left, infix_ops::not_equal, ValueType::Int);
    current = add_infix_builtin(std::move(current), "<", precedence::comparison, Associativity::Left, infix_ops::less, ValueType::Int);
    current = add_infix_builtin(std::move(current), "<=", precedence::comparison, Associativity::Left, infix_ops::less_equal, ValueType::Int);
    current = add_infix_builtin(std::move(current), ">", precedence::comparison, Associativity::Left, infix_ops::greater, ValueType::Int);
    current = add_infix_builtin(std::move(current), ">=", precedence::comparison, Associativity::Left, infix_ops::greater_equal, ValueType::Int);
    return current;
}

template <class Context>
constexpr auto register_logical_infix(Context context)
{
    auto current = std::move(context);
    current = add_infix_builtin(
        std::move(current),
        "&&",
        precedence::logical_and,
        Associativity::Left,
        infix_ops::logical_and,
        ValueType::Int,
        semantics::logical_short_circuit());
    current = add_infix_builtin(
        std::move(current),
        "||",
        precedence::logical_or,
        Associativity::Left,
        infix_ops::logical_or,
        ValueType::Int,
        semantics::logical_short_circuit());
    return current;
}

template <class Context>
constexpr auto register_prefix_operators(Context context)
{
    auto current = std::move(context);
    current = add_prefix_builtin(std::move(current), "+", precedence::prefix, prefix_ops::identity);
    current = add_prefix_builtin(std::move(current), "-", precedence::prefix, prefix_ops::negate);
    current = add_prefix_builtin(std::move(current), "!", precedence::prefix, prefix_ops::logical_not, ValueType::Int);
    current = add_prefix_builtin(std::move(current), "abs", precedence::prefix, prefix_ops::abs);
    current = add_prefix_builtin(std::move(current), "sqrt", precedence::prefix, prefix_ops::sqrt);
    return current;
}

template <class Context>
constexpr auto register_postfix_operators(Context context)
{
    return add_postfix_builtin(std::move(context), "!", precedence::postfix, postfix_ops::factorial);
}

template <class Context>
constexpr auto register_scalar_functions(Context context)
{
    auto current = std::move(context);
    current = add_unary_function_builtin(std::move(current), "sin", function_ops::sin);
    current = add_unary_function_builtin(std::move(current), "cos", function_ops::cos);
    current = add_unary_function_builtin(std::move(current), "tan", function_ops::tan);
    current = add_unary_function_builtin(std::move(current), "asin", function_ops::asin);
    current = add_unary_function_builtin(std::move(current), "acos", function_ops::acos);
    current = add_unary_function_builtin(std::move(current), "atan", function_ops::atan);
    current = add_unary_function_builtin(std::move(current), "sqrt", function_ops::sqrt);
    current = add_unary_function_builtin(std::move(current), "ceil", function_ops::ceil);
    current = add_unary_function_builtin(std::move(current), "floor", function_ops::floor);
    current = add_unary_function_builtin(std::move(current), "round", function_ops::round);
    current = add_unary_function_builtin(std::move(current), "exp", function_ops::exp);
    current = add_unary_function_builtin(std::move(current), "abs", function_ops::abs);
    current = add_unary_function_builtin(std::move(current), "rad", function_ops::radians);
    current = add_unary_function_builtin(std::move(current), "deg", function_ops::degrees);
    return current;
}

template <class Context>
constexpr auto register_variadic_functions(Context context)
{
    auto current = std::move(context);
    current = add_function_builtin(
        std::move(current),
        "max",
        variadic_ops::maximum,
        1,
        internal::k_unbounded_arity);
    current = add_function_builtin(
        std::move(current),
        "min",
        variadic_ops::minimum,
        1,
        internal::k_unbounded_arity);
    return current;
}

template <class Context>
constexpr auto register_constants(Context context)
{
    auto current = std::move(context);
    current = add_constant(std::move(current), "PI", MathValue(static_cast<double>(constants::pi)));
    current = add_constant(std::move(current), "E", MathValue(static_cast<double>(constants::e)));
    return current;
}

template <class Context>
constexpr auto apply(Context context)
{
    auto current = std::move(context);
    current = register_arithmetic_infix(std::move(current));
    current = register_comparison_infix(std::move(current));
    current = register_logical_infix(std::move(current));
    current = register_prefix_operators(std::move(current));
    current = register_postfix_operators(std::move(current));
    current = register_scalar_functions(std::move(current));
    current = register_variadic_functions(std::move(current));
    current = register_constants(std::move(current));
    return current;
}

inline const MathContext& default_math_context_ref()
{
    static const MathContext context = apply(MathContext{});
    return context;
}

} // namespace detail

} // namespace string_math::builtin::default_math

namespace string_math::builtin
{

template <class Context = MathContext>
constexpr auto with_default_math(Context context = {})
{
    return default_math::detail::apply(std::move(context));
}

inline MathContext default_math_context()
{
    return with_default_math(MathContext{});
}

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
constexpr auto default_math_compile_time_context()
{
    return with_default_math(MathContext::compile_time<16, 24, 32, 12, 8>());
}
#endif

inline Result<MathValue> try_evaluate(std::string_view expression)
{
    return string_math::try_evaluate(expression, default_math::detail::default_math_context_ref());
}

inline MathValue evaluate(std::string_view expression)
{
    return string_math::evaluate(expression, default_math::detail::default_math_context_ref());
}

inline MathExpr to_math_expr(std::string expression)
{
    return MathExpr(std::move(expression), default_math::detail::default_math_context_ref());
}

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
template <std::size_t N>
constexpr MathValue evaluate(const char (&expression)[N])
{
    auto context = default_math_compile_time_context();
    return internal::ConstexprParser<decltype(context)>(std::string_view(expression, N - 1), &context).parse();
}

constexpr MathValue operator""_math(const char* expression, std::size_t size)
{
    auto context = default_math_compile_time_context();
    return internal::ConstexprParser<decltype(context)>(std::string_view(expression, size), &context).parse();
}
#else
inline MathValue operator""_math(const char* expression, std::size_t size)
{
    return evaluate(std::string_view(expression, size));
}
#endif

} // namespace string_math::builtin
