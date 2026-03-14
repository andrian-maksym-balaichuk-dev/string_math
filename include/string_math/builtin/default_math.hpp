#pragma once

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <string_math/context/context.hpp>
#include <string_math/context/static_context.hpp>
#include <string_math/expression/evaluate.hpp>
#include <string_math/internal/arithmetic.hpp>

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

struct add_policy_fn
{
    template <class T>
    constexpr Result<MathValue> operator()(T left, T right, const EvaluationPolicy& policy, std::string_view token) const
    {
        return add_policy(left, right, policy, token);
    }
};

struct subtract_policy_fn
{
    template <class T>
    constexpr Result<MathValue> operator()(T left, T right, const EvaluationPolicy& policy, std::string_view token) const
    {
        return subtract_policy(left, right, policy, token);
    }
};

struct multiply_policy_fn
{
    template <class T>
    constexpr Result<MathValue> operator()(T left, T right, const EvaluationPolicy& policy, std::string_view token) const
    {
        return multiply_policy(left, right, policy, token);
    }
};

constexpr bool truthy(const MathValue& value) noexcept
{
    return value.visit([](const auto& current) { return current != 0; });
}

constexpr MathValue cx_prefix_identity(const void*, const MathValue* arguments, std::size_t)
{
    return arguments[0];
}

constexpr MathValue cx_prefix_negate(const void*, const MathValue* arguments, std::size_t)
{
    return arguments[0].visit([](const auto& current) { return MathValue(-current); });
}

constexpr MathValue cx_prefix_logical_not(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(truthy(arguments[0]) ? 0 : 1);
}

inline MathValue cx_prefix_abs(const void*, const MathValue* arguments, std::size_t)
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

inline MathValue cx_prefix_sqrt(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(std::sqrt(arguments[0].to_double()));
}

constexpr MathValue cx_infix_add(const void*, const MathValue* arguments, std::size_t)
{
    return arguments[0] + arguments[1];
}

constexpr MathValue cx_infix_subtract(const void*, const MathValue* arguments, std::size_t)
{
    return arguments[0] - arguments[1];
}

constexpr MathValue cx_infix_multiply(const void*, const MathValue* arguments, std::size_t)
{
    return arguments[0] * arguments[1];
}

constexpr MathValue cx_infix_divide(const void*, const MathValue* arguments, std::size_t)
{
    if (arguments[1] == 0)
    {
        throw std::domain_error("string_math: division by zero");
    }
    return arguments[0] / arguments[1];
}

constexpr MathValue cx_infix_modulo(const void*, const MathValue* arguments, std::size_t)
{
    if (arguments[1] == 0)
    {
        throw std::domain_error("string_math: modulo by zero");
    }
    return arguments[0] % arguments[1];
}

inline MathValue cx_infix_power(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(std::pow(arguments[0].to_double(), arguments[1].to_double()));
}

inline MathValue cx_infix_log(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(std::log(arguments[1].to_double()) / std::log(arguments[0].to_double()));
}

constexpr MathValue cx_infix_equal(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(arguments[0] == arguments[1] ? 1 : 0);
}

constexpr MathValue cx_infix_not_equal(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(arguments[0] != arguments[1] ? 1 : 0);
}

constexpr MathValue cx_infix_less(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(arguments[0] < arguments[1] ? 1 : 0);
}

constexpr MathValue cx_infix_less_equal(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(arguments[0] <= arguments[1] ? 1 : 0);
}

constexpr MathValue cx_infix_greater(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(arguments[0] > arguments[1] ? 1 : 0);
}

constexpr MathValue cx_infix_greater_equal(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(arguments[0] >= arguments[1] ? 1 : 0);
}

constexpr MathValue cx_infix_logical_and(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(truthy(arguments[0]) && truthy(arguments[1]) ? 1 : 0);
}

constexpr MathValue cx_infix_logical_or(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(truthy(arguments[0]) || truthy(arguments[1]) ? 1 : 0);
}

constexpr MathValue cx_infix_max(const void*, const MathValue* arguments, std::size_t)
{
    return arguments[0] > arguments[1] ? arguments[0] : arguments[1];
}

constexpr MathValue cx_infix_min(const void*, const MathValue* arguments, std::size_t)
{
    return arguments[0] < arguments[1] ? arguments[0] : arguments[1];
}

inline MathValue cx_postfix_factorial(const void*, const MathValue* arguments, std::size_t)
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

inline MathValue cx_function_sin(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(std::sin(arguments[0].to_double()));
}

inline MathValue cx_function_cos(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(std::cos(arguments[0].to_double()));
}

inline MathValue cx_function_tan(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(std::tan(arguments[0].to_double()));
}

inline MathValue cx_function_asin(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(std::asin(arguments[0].to_double()));
}

inline MathValue cx_function_acos(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(std::acos(arguments[0].to_double()));
}

inline MathValue cx_function_atan(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(std::atan(arguments[0].to_double()));
}

inline MathValue cx_function_sqrt(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(std::sqrt(arguments[0].to_double()));
}

inline MathValue cx_function_ceil(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(std::ceil(arguments[0].to_double()));
}

inline MathValue cx_function_floor(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(std::floor(arguments[0].to_double()));
}

inline MathValue cx_function_round(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(std::round(arguments[0].to_double()));
}

inline MathValue cx_function_exp(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(std::exp(arguments[0].to_double()));
}

inline MathValue cx_function_abs(const void*, const MathValue* arguments, std::size_t)
{
    return cx_prefix_abs(nullptr, arguments, 1);
}

inline MathValue cx_function_rad(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(arguments[0].to_double() * (3.14159265358979323846 / 180.0));
}

inline MathValue cx_function_deg(const void*, const MathValue* arguments, std::size_t)
{
    return MathValue(arguments[0].to_double() * (180.0 / 3.14159265358979323846));
}

constexpr MathValue cx_variadic_max(const void*, const MathValue* arguments, std::size_t count)
{
    MathValue current = arguments[0];
    for (std::size_t index = 1; index < count; ++index)
    {
        if (arguments[index] > current)
        {
            current = arguments[index];
        }
    }
    return current;
}

constexpr MathValue cx_variadic_min(const void*, const MathValue* arguments, std::size_t count)
{
    MathValue current = arguments[0];
    for (std::size_t index = 1; index < count; ++index)
    {
        if (arguments[index] < current)
        {
            current = arguments[index];
        }
    }
    return current;
}

template <class Handler>
constexpr Result<MathValue> dispatch_binary_policy(
    const MathValue* arguments,
    std::size_t count,
    const EvaluationPolicy& policy,
    std::string_view token,
    Handler handler)
{
    if (count != 2)
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

constexpr Result<MathValue> cx_add_policy(
    const void*,
    const MathValue* arguments,
    std::size_t count,
    const EvaluationPolicy& policy,
    std::string_view token)
{
    return dispatch_binary_policy(arguments, count, policy, token, add_policy_fn{});
}

constexpr Result<MathValue> cx_subtract_policy(
    const void*,
    const MathValue* arguments,
    std::size_t count,
    const EvaluationPolicy& policy,
    std::string_view token)
{
    return dispatch_binary_policy(arguments, count, policy, token, subtract_policy_fn{});
}

constexpr Result<MathValue> cx_multiply_policy(
    const void*,
    const MathValue* arguments,
    std::size_t count,
    const EvaluationPolicy& policy,
    std::string_view token)
{
    return dispatch_binary_policy(arguments, count, policy, token, multiply_policy_fn{});
}

enum class BuiltinOp
{
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Power,
    Log,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    LogicalAnd,
    LogicalOr,
    InfixMax,
    InfixMin,
    PrefixIdentity,
    PrefixNegate,
    PrefixLogicalNot,
    PostfixFactorial,
    FunctionSin,
    FunctionCos,
    FunctionTan,
    FunctionAsin,
    FunctionAcos,
    FunctionAtan,
    FunctionSqrt,
    FunctionCeil,
    FunctionFloor,
    FunctionRound,
    FunctionExp,
    FunctionAbs,
    FunctionRad,
    FunctionDeg,
    VariadicMax,
    VariadicMin,
    PrefixAbs,
    PrefixSqrt,
};

struct BuiltinCallableSpec
{
    std::string_view name_or_symbol;
    internal::CallableKind kind;
    BuiltinOp op;
    int precedence{0};
    Associativity associativity{Associativity::Left};
    std::size_t min_arity{0};
    std::size_t max_arity{0};
    ValueType result_type{ValueType::Double};
    CallableSemantics semantics{};
};

inline constexpr std::array<internal::ValueDescriptorView, 2> k_builtin_values{{
    {"PI", MathValue(3.14159265358979323846)},
    {"E", MathValue(2.71828182845904523536)},
}};

inline constexpr std::array<BuiltinCallableSpec, 39> k_builtin_callables{{
    {"+", internal::CallableKind::InfixOperator, BuiltinOp::Add, precedence::additive, Associativity::Left, 2, 2, ValueType::Double, semantics::overflow_sensitive()},
    {"-", internal::CallableKind::InfixOperator, BuiltinOp::Subtract, precedence::additive, Associativity::Left, 2, 2, ValueType::Double, semantics::overflow_sensitive()},
    {"*", internal::CallableKind::InfixOperator, BuiltinOp::Multiply, precedence::multiplicative, Associativity::Left, 2, 2, ValueType::Double, semantics::overflow_sensitive()},
    {"/", internal::CallableKind::InfixOperator, BuiltinOp::Divide, precedence::multiplicative, Associativity::Left, 2, 2, ValueType::Double, semantics::pure()},
    {"%", internal::CallableKind::InfixOperator, BuiltinOp::Modulo, precedence::multiplicative, Associativity::Left, 2, 2, ValueType::Double, semantics::pure()},
    {"^", internal::CallableKind::InfixOperator, BuiltinOp::Power, precedence::power, Associativity::Right, 2, 2, ValueType::Double, semantics::pure()},
    {"log", internal::CallableKind::InfixOperator, BuiltinOp::Log, precedence::power, Associativity::Left, 2, 2, ValueType::Double, semantics::pure()},
    {"==", internal::CallableKind::InfixOperator, BuiltinOp::Equal, precedence::comparison, Associativity::Left, 2, 2, ValueType::Int, semantics::pure()},
    {"!=", internal::CallableKind::InfixOperator, BuiltinOp::NotEqual, precedence::comparison, Associativity::Left, 2, 2, ValueType::Int, semantics::pure()},
    {"<", internal::CallableKind::InfixOperator, BuiltinOp::Less, precedence::comparison, Associativity::Left, 2, 2, ValueType::Int, semantics::pure()},
    {"<=", internal::CallableKind::InfixOperator, BuiltinOp::LessEqual, precedence::comparison, Associativity::Left, 2, 2, ValueType::Int, semantics::pure()},
    {">", internal::CallableKind::InfixOperator, BuiltinOp::Greater, precedence::comparison, Associativity::Left, 2, 2, ValueType::Int, semantics::pure()},
    {">=", internal::CallableKind::InfixOperator, BuiltinOp::GreaterEqual, precedence::comparison, Associativity::Left, 2, 2, ValueType::Int, semantics::pure()},
    {"&&", internal::CallableKind::InfixOperator, BuiltinOp::LogicalAnd, precedence::logical_and, Associativity::Left, 2, 2, ValueType::Int, semantics::logical_short_circuit()},
    {"||", internal::CallableKind::InfixOperator, BuiltinOp::LogicalOr, precedence::logical_or, Associativity::Left, 2, 2, ValueType::Int, semantics::logical_short_circuit()},
    {"max", internal::CallableKind::InfixOperator, BuiltinOp::InfixMax, precedence::power, Associativity::Left, 2, 2, ValueType::Double, semantics::pure()},
    {"min", internal::CallableKind::InfixOperator, BuiltinOp::InfixMin, precedence::power, Associativity::Left, 2, 2, ValueType::Double, semantics::pure()},
    {"+", internal::CallableKind::PrefixOperator, BuiltinOp::PrefixIdentity, precedence::prefix, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"-", internal::CallableKind::PrefixOperator, BuiltinOp::PrefixNegate, precedence::prefix, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"!", internal::CallableKind::PrefixOperator, BuiltinOp::PrefixLogicalNot, precedence::prefix, Associativity::Left, 1, 1, ValueType::Int, semantics::pure()},
    {"!", internal::CallableKind::PostfixOperator, BuiltinOp::PostfixFactorial, precedence::postfix, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"sin", internal::CallableKind::Function, BuiltinOp::FunctionSin, 0, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"cos", internal::CallableKind::Function, BuiltinOp::FunctionCos, 0, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"tan", internal::CallableKind::Function, BuiltinOp::FunctionTan, 0, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"asin", internal::CallableKind::Function, BuiltinOp::FunctionAsin, 0, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"acos", internal::CallableKind::Function, BuiltinOp::FunctionAcos, 0, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"atan", internal::CallableKind::Function, BuiltinOp::FunctionAtan, 0, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"sqrt", internal::CallableKind::Function, BuiltinOp::FunctionSqrt, 0, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"ceil", internal::CallableKind::Function, BuiltinOp::FunctionCeil, 0, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"floor", internal::CallableKind::Function, BuiltinOp::FunctionFloor, 0, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"round", internal::CallableKind::Function, BuiltinOp::FunctionRound, 0, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"exp", internal::CallableKind::Function, BuiltinOp::FunctionExp, 0, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"abs", internal::CallableKind::Function, BuiltinOp::FunctionAbs, 0, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"rad", internal::CallableKind::Function, BuiltinOp::FunctionRad, 0, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"deg", internal::CallableKind::Function, BuiltinOp::FunctionDeg, 0, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"max", internal::CallableKind::Function, BuiltinOp::VariadicMax, 0, Associativity::Left, 1, internal::k_unbounded_arity, ValueType::Double, semantics::pure()},
    {"min", internal::CallableKind::Function, BuiltinOp::VariadicMin, 0, Associativity::Left, 1, internal::k_unbounded_arity, ValueType::Double, semantics::pure()},
    {"abs", internal::CallableKind::PrefixOperator, BuiltinOp::PrefixAbs, precedence::prefix, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
    {"sqrt", internal::CallableKind::PrefixOperator, BuiltinOp::PrefixSqrt, precedence::prefix, Associativity::Left, 1, 1, ValueType::Double, semantics::pure()},
}};

inline MathContext apply_runtime_builtin(MathContext context, const BuiltinCallableSpec& spec)
{
    switch (spec.op)
    {
    case BuiltinOp::Add:
        context.template add_infix_operator_with_policy_for<STRING_MATH_ALL_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto left, auto right) { return left + right; },
            spec.precedence,
            add_policy_fn{},
            spec.associativity,
            spec.semantics);
        return context;
    case BuiltinOp::Subtract:
        context.template add_infix_operator_with_policy_for<STRING_MATH_ALL_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto left, auto right) { return left - right; },
            spec.precedence,
            subtract_policy_fn{},
            spec.associativity,
            spec.semantics);
        return context;
    case BuiltinOp::Multiply:
        context.template add_infix_operator_with_policy_for<STRING_MATH_ALL_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto left, auto right) { return left * right; },
            spec.precedence,
            multiply_policy_fn{},
            spec.associativity,
            spec.semantics);
        return context;
    case BuiltinOp::Divide:
        context.template add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto left, auto right) {
                if (right == 0)
                {
                    throw std::domain_error("string_math: division by zero");
                }
                return left / right;
            },
            spec.precedence,
            spec.associativity,
            spec.semantics);
        return context;
    case BuiltinOp::Modulo:
        context.template add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto left, auto right) {
                if (right == 0)
                {
                    throw std::domain_error("string_math: modulo by zero");
                }
                using value_t = std::decay_t<decltype(left)>;
                if constexpr (std::is_integral_v<value_t>)
                {
                    return left % right;
                }
                else
                {
                    return std::fmod(left, right);
                }
            },
            spec.precedence,
            spec.associativity,
            spec.semantics);
        return context;
    case BuiltinOp::Power:
        context.template add_infix_operator_for<double, long double>(
            spec.name_or_symbol,
            [](auto left, auto right) { return std::pow(left, right); },
            spec.precedence,
            spec.associativity,
            spec.semantics);
        return context;
    case BuiltinOp::Log:
        context.template add_infix_operator_for<double, long double>(
            spec.name_or_symbol,
            [](auto base, auto value) { return std::log(value) / std::log(base); },
            spec.precedence,
            spec.associativity,
            spec.semantics);
        return context;
    case BuiltinOp::Equal:
        context.template add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto left, auto right) { return left == right ? 1 : 0; },
            spec.precedence,
            spec.associativity,
            spec.semantics);
        return context;
    case BuiltinOp::NotEqual:
        context.template add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto left, auto right) { return left != right ? 1 : 0; },
            spec.precedence,
            spec.associativity,
            spec.semantics);
        return context;
    case BuiltinOp::Less:
        context.template add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto left, auto right) { return left < right ? 1 : 0; },
            spec.precedence,
            spec.associativity,
            spec.semantics);
        return context;
    case BuiltinOp::LessEqual:
        context.template add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto left, auto right) { return left <= right ? 1 : 0; },
            spec.precedence,
            spec.associativity,
            spec.semantics);
        return context;
    case BuiltinOp::Greater:
        context.template add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto left, auto right) { return left > right ? 1 : 0; },
            spec.precedence,
            spec.associativity,
            spec.semantics);
        return context;
    case BuiltinOp::GreaterEqual:
        context.template add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto left, auto right) { return left >= right ? 1 : 0; },
            spec.precedence,
            spec.associativity,
            spec.semantics);
        return context;
    case BuiltinOp::LogicalAnd:
        context.template add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto left, auto right) { return (left != 0 && right != 0) ? 1 : 0; },
            spec.precedence,
            spec.associativity,
            spec.semantics);
        return context;
    case BuiltinOp::LogicalOr:
        context.template add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto left, auto right) { return (left != 0 || right != 0) ? 1 : 0; },
            spec.precedence,
            spec.associativity,
            spec.semantics);
        return context;
    case BuiltinOp::InfixMax:
        context.template add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto left, auto right) { return left > right ? left : right; },
            spec.precedence,
            spec.associativity,
            spec.semantics);
        return context;
    case BuiltinOp::InfixMin:
        context.template add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto left, auto right) { return left < right ? left : right; },
            spec.precedence,
            spec.associativity,
            spec.semantics);
        return context;
    case BuiltinOp::PrefixIdentity:
        context.template add_prefix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto value) { return value; },
            spec.precedence,
            spec.semantics);
        return context;
    case BuiltinOp::PrefixNegate:
        context.template add_prefix_operator_for<STRING_MATH_SIGNED_AND_FLOATING_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto value) { return -value; },
            spec.precedence,
            spec.semantics);
        return context;
    case BuiltinOp::PrefixLogicalNot:
        context.template add_prefix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto value) { return value == 0 ? 1 : 0; },
            spec.precedence,
            spec.semantics);
        return context;
    case BuiltinOp::PostfixFactorial:
        context.template add_postfix_operator_overloads<STRING_MATH_SIGNED_FACTORIAL_SIGNATURES, STRING_MATH_UNSIGNED_FACTORIAL_SIGNATURES>(
            spec.name_or_symbol,
            [](auto value) {
                using value_t = std::decay_t<decltype(value)>;
                return static_cast<value_t>(internal::factorial_value(static_cast<long double>(value)));
            },
            spec.precedence,
            spec.semantics);
        context.template add_postfix_operator_for<double, long double>(
            spec.name_or_symbol,
            [](auto value) { return internal::factorial_value(value); },
            spec.precedence,
            spec.semantics);
        return context;
    case BuiltinOp::FunctionSin:
        context.template add_function_for<float, double, long double>(spec.name_or_symbol, [](auto value) { return std::sin(value); }, spec.semantics);
        return context;
    case BuiltinOp::FunctionCos:
        context.template add_function_for<float, double, long double>(spec.name_or_symbol, [](auto value) { return std::cos(value); }, spec.semantics);
        return context;
    case BuiltinOp::FunctionTan:
        context.template add_function_for<float, double, long double>(spec.name_or_symbol, [](auto value) { return std::tan(value); }, spec.semantics);
        return context;
    case BuiltinOp::FunctionAsin:
        context.template add_function_for<float, double, long double>(spec.name_or_symbol, [](auto value) { return std::asin(value); }, spec.semantics);
        return context;
    case BuiltinOp::FunctionAcos:
        context.template add_function_for<float, double, long double>(spec.name_or_symbol, [](auto value) { return std::acos(value); }, spec.semantics);
        return context;
    case BuiltinOp::FunctionAtan:
        context.template add_function_for<float, double, long double>(spec.name_or_symbol, [](auto value) { return std::atan(value); }, spec.semantics);
        return context;
    case BuiltinOp::FunctionSqrt:
        context.template add_function_for<float, double, long double>(spec.name_or_symbol, [](auto value) { return std::sqrt(value); }, spec.semantics);
        return context;
    case BuiltinOp::FunctionCeil:
        context.template add_function_for<float, double, long double>(spec.name_or_symbol, [](auto value) { return std::ceil(value); }, spec.semantics);
        return context;
    case BuiltinOp::FunctionFloor:
        context.template add_function_for<float, double, long double>(spec.name_or_symbol, [](auto value) { return std::floor(value); }, spec.semantics);
        return context;
    case BuiltinOp::FunctionRound:
        context.template add_function_for<float, double, long double>(spec.name_or_symbol, [](auto value) { return std::round(value); }, spec.semantics);
        return context;
    case BuiltinOp::FunctionExp:
        context.template add_function_for<float, double, long double>(spec.name_or_symbol, [](auto value) { return std::exp(value); }, spec.semantics);
        return context;
    case BuiltinOp::FunctionAbs:
        context.template add_function_for<STRING_MATH_SIGNED_AND_FLOATING_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto value) { return std::abs(value); },
            spec.semantics);
        return context;
    case BuiltinOp::FunctionRad:
        context.template add_function<double(double)>(
            spec.name_or_symbol,
            [](double value) { return value * (3.14159265358979323846 / 180.0); },
            spec.semantics);
        return context;
    case BuiltinOp::FunctionDeg:
        context.template add_function<double(double)>(
            spec.name_or_symbol,
            [](double value) { return value * (180.0 / 3.14159265358979323846); },
            spec.semantics);
        return context;
    case BuiltinOp::VariadicMax:
        context.add_variadic_function(
            spec.name_or_symbol,
            spec.min_arity,
            [](const std::vector<MathValue>& arguments) { return cx_variadic_max(nullptr, arguments.data(), arguments.size()); },
            spec.semantics);
        return context;
    case BuiltinOp::VariadicMin:
        context.add_variadic_function(
            spec.name_or_symbol,
            spec.min_arity,
            [](const std::vector<MathValue>& arguments) { return cx_variadic_min(nullptr, arguments.data(), arguments.size()); },
            spec.semantics);
        return context;
    case BuiltinOp::PrefixAbs:
        context.template add_prefix_operator_for<STRING_MATH_SIGNED_AND_FLOATING_VALUE_TYPES>(
            spec.name_or_symbol,
            [](auto value) { return std::abs(value); },
            spec.precedence,
            spec.semantics);
        return context;
    case BuiltinOp::PrefixSqrt:
        context.template add_prefix_operator_for<float, double, long double>(
            spec.name_or_symbol,
            [](auto value) { return std::sqrt(value); },
            spec.precedence,
            spec.semantics);
        return context;
    }

    return context;
}

template <std::size_t Index, class Context>
constexpr auto apply_constexpr_builtin(Context context)
{
    constexpr BuiltinCallableSpec spec = k_builtin_callables[Index];

    if constexpr (spec.op == BuiltinOp::Add)
    {
        return context.add_generic_infix_operator(
            spec.name_or_symbol,
            spec.precedence,
            spec.associativity,
            &cx_infix_add,
            spec.result_type,
            spec.semantics,
            &cx_add_policy);
    }
    else if constexpr (spec.op == BuiltinOp::Subtract)
    {
        return context.add_generic_infix_operator(
            spec.name_or_symbol,
            spec.precedence,
            spec.associativity,
            &cx_infix_subtract,
            spec.result_type,
            spec.semantics,
            &cx_subtract_policy);
    }
    else if constexpr (spec.op == BuiltinOp::Multiply)
    {
        return context.add_generic_infix_operator(
            spec.name_or_symbol,
            spec.precedence,
            spec.associativity,
            &cx_infix_multiply,
            spec.result_type,
            spec.semantics,
            &cx_multiply_policy);
    }
    else if constexpr (spec.op == BuiltinOp::Divide)
    {
        return context.add_generic_infix_operator(spec.name_or_symbol, spec.precedence, spec.associativity, &cx_infix_divide, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::Modulo)
    {
        return context.add_generic_infix_operator(spec.name_or_symbol, spec.precedence, spec.associativity, &cx_infix_modulo, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::Power)
    {
        return context.add_generic_infix_operator(spec.name_or_symbol, spec.precedence, spec.associativity, &cx_infix_power, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::Log)
    {
        return context.add_generic_infix_operator(spec.name_or_symbol, spec.precedence, spec.associativity, &cx_infix_log, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::Equal)
    {
        return context.add_generic_infix_operator(spec.name_or_symbol, spec.precedence, spec.associativity, &cx_infix_equal, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::NotEqual)
    {
        return context.add_generic_infix_operator(spec.name_or_symbol, spec.precedence, spec.associativity, &cx_infix_not_equal, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::Less)
    {
        return context.add_generic_infix_operator(spec.name_or_symbol, spec.precedence, spec.associativity, &cx_infix_less, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::LessEqual)
    {
        return context.add_generic_infix_operator(spec.name_or_symbol, spec.precedence, spec.associativity, &cx_infix_less_equal, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::Greater)
    {
        return context.add_generic_infix_operator(spec.name_or_symbol, spec.precedence, spec.associativity, &cx_infix_greater, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::GreaterEqual)
    {
        return context.add_generic_infix_operator(spec.name_or_symbol, spec.precedence, spec.associativity, &cx_infix_greater_equal, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::LogicalAnd)
    {
        return context.add_generic_infix_operator(spec.name_or_symbol, spec.precedence, spec.associativity, &cx_infix_logical_and, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::LogicalOr)
    {
        return context.add_generic_infix_operator(spec.name_or_symbol, spec.precedence, spec.associativity, &cx_infix_logical_or, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::InfixMax)
    {
        return context.add_generic_infix_operator(spec.name_or_symbol, spec.precedence, spec.associativity, &cx_infix_max, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::InfixMin)
    {
        return context.add_generic_infix_operator(spec.name_or_symbol, spec.precedence, spec.associativity, &cx_infix_min, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::PrefixIdentity)
    {
        return context.add_generic_prefix_operator(spec.name_or_symbol, spec.precedence, &cx_prefix_identity, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::PrefixNegate)
    {
        return context.add_generic_prefix_operator(spec.name_or_symbol, spec.precedence, &cx_prefix_negate, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::PrefixLogicalNot)
    {
        return context.add_generic_prefix_operator(spec.name_or_symbol, spec.precedence, &cx_prefix_logical_not, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::PostfixFactorial)
    {
        return context.add_generic_postfix_operator(spec.name_or_symbol, spec.precedence, &cx_postfix_factorial, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::FunctionSin)
    {
        return context.add_generic_function(spec.name_or_symbol, &cx_function_sin, spec.min_arity, spec.max_arity, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::FunctionCos)
    {
        return context.add_generic_function(spec.name_or_symbol, &cx_function_cos, spec.min_arity, spec.max_arity, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::FunctionTan)
    {
        return context.add_generic_function(spec.name_or_symbol, &cx_function_tan, spec.min_arity, spec.max_arity, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::FunctionAsin)
    {
        return context.add_generic_function(spec.name_or_symbol, &cx_function_asin, spec.min_arity, spec.max_arity, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::FunctionAcos)
    {
        return context.add_generic_function(spec.name_or_symbol, &cx_function_acos, spec.min_arity, spec.max_arity, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::FunctionAtan)
    {
        return context.add_generic_function(spec.name_or_symbol, &cx_function_atan, spec.min_arity, spec.max_arity, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::FunctionSqrt)
    {
        return context.add_generic_function(spec.name_or_symbol, &cx_function_sqrt, spec.min_arity, spec.max_arity, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::FunctionCeil)
    {
        return context.add_generic_function(spec.name_or_symbol, &cx_function_ceil, spec.min_arity, spec.max_arity, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::FunctionFloor)
    {
        return context.add_generic_function(spec.name_or_symbol, &cx_function_floor, spec.min_arity, spec.max_arity, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::FunctionRound)
    {
        return context.add_generic_function(spec.name_or_symbol, &cx_function_round, spec.min_arity, spec.max_arity, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::FunctionExp)
    {
        return context.add_generic_function(spec.name_or_symbol, &cx_function_exp, spec.min_arity, spec.max_arity, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::FunctionAbs)
    {
        return context.add_generic_function(spec.name_or_symbol, &cx_function_abs, spec.min_arity, spec.max_arity, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::FunctionRad)
    {
        return context.add_generic_function(spec.name_or_symbol, &cx_function_rad, spec.min_arity, spec.max_arity, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::FunctionDeg)
    {
        return context.add_generic_function(spec.name_or_symbol, &cx_function_deg, spec.min_arity, spec.max_arity, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::VariadicMax)
    {
        return context.add_variadic_function(spec.name_or_symbol, spec.min_arity, spec.max_arity, &cx_variadic_max, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::VariadicMin)
    {
        return context.add_variadic_function(spec.name_or_symbol, spec.min_arity, spec.max_arity, &cx_variadic_min, spec.result_type, spec.semantics);
    }
    else if constexpr (spec.op == BuiltinOp::PrefixAbs)
    {
        return context.add_generic_prefix_operator(spec.name_or_symbol, spec.precedence, &cx_prefix_abs, spec.result_type, spec.semantics);
    }
    else
    {
        return context.add_generic_prefix_operator(spec.name_or_symbol, spec.precedence, &cx_prefix_sqrt, spec.result_type, spec.semantics);
    }
}

template <std::size_t Index = 0, class Context>
constexpr auto apply_builtin_values(Context context)
{
    if constexpr (Index == k_builtin_values.size())
    {
        return context;
    }
    else if constexpr (is_constexpr_context_v<Context>)
    {
        auto next = context.set_value(k_builtin_values[Index].name, k_builtin_values[Index].value);
        return apply_builtin_values<Index + 1>(std::move(next));
    }
    else
    {
        context.set_value(k_builtin_values[Index].name, k_builtin_values[Index].value);
        return apply_builtin_values<Index + 1>(std::move(context));
    }
}

template <std::size_t Index = 0, class Context>
constexpr auto apply_builtin_callables(Context context)
{
    if constexpr (Index == k_builtin_callables.size())
    {
        return context;
    }
    else if constexpr (is_constexpr_context_v<Context>)
    {
        auto next = apply_constexpr_builtin<Index>(std::move(context));
        return apply_builtin_callables<Index + 1>(std::move(next));
    }
    else
    {
        return apply_builtin_callables<Index + 1>(apply_runtime_builtin(std::move(context), k_builtin_callables[Index]));
    }
}

template <class Context>
constexpr auto apply(Context context)
{
    auto with_callables = apply_builtin_callables(std::move(context));
    return apply_builtin_values(std::move(with_callables));
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
    return with_default_math(MathContext::compile_time());
}
#endif

} // namespace string_math::builtin

namespace string_math
{

inline Result<MathValue> try_evaluate(std::string_view expression)
{
    return try_evaluate(expression, builtin::default_math::detail::default_math_context_ref());
}

inline MathValue evaluate(std::string_view expression)
{
    return evaluate(expression, builtin::default_math::detail::default_math_context_ref());
}

inline MathExpr to_math_expr(std::string expression)
{
    return MathExpr(std::move(expression), builtin::default_math::detail::default_math_context_ref());
}

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
inline constexpr auto k_default_math_compile_time_context = builtin::default_math_compile_time_context();

template <std::size_t N>
constexpr MathValue evaluate(const char (&expression)[N])
{
    return internal::ConstexprParser<decltype(k_default_math_compile_time_context)>(
        std::string_view(expression, N - 1),
        &k_default_math_compile_time_context)
        .parse();
}

constexpr MathValue operator""_math(const char* expression, std::size_t size)
{
    return internal::ConstexprParser<decltype(k_default_math_compile_time_context)>(
        std::string_view(expression, size),
        &k_default_math_compile_time_context)
        .parse();
}
#else
inline MathValue operator""_math(const char* expression, std::size_t size)
{
    return evaluate(std::string_view(expression, size));
}
#endif

} // namespace string_math
