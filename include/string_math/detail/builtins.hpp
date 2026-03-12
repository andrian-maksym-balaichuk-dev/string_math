#pragma once

#include <array>
#include <cmath>
#include <stdexcept>
#include <string_view>

#include <string_math/context.hpp>

namespace string_math
{

namespace detail
{

struct BuiltinPrefixOperatorSpec
{
    std::string_view symbol;
    int precedence{Precedence::Prefix};
    CallableSemantics semantics{};
    MathValue (*invoke)(const MathValue&) = nullptr;
};

struct BuiltinPostfixOperatorSpec
{
    std::string_view symbol;
    int precedence{Precedence::Postfix};
    CallableSemantics semantics{};
    MathValue (*invoke)(const MathValue&) = nullptr;
};

struct BuiltinInfixOperatorSpec
{
    std::string_view symbol;
    int precedence{0};
    Associativity associativity{Associativity::Left};
    CallableSemantics semantics{};
    MathValue (*invoke)(const MathValue&, const MathValue&) = nullptr;
};

struct BuiltinFunctionSpec
{
    std::string_view name;
    CallableSemantics semantics{};
    MathValue (*invoke)(const MathValue*, std::size_t) = nullptr;
};

constexpr bool builtin_truthy(const MathValue& value) noexcept
{
    return value.visit([](const auto& current) { return current != 0; });
}

constexpr MathValue builtin_prefix_identity(const MathValue& value)
{
    return value;
}

constexpr MathValue builtin_prefix_negate(const MathValue& value)
{
    return value.visit([](const auto& current) { return MathValue(-current); });
}

constexpr MathValue builtin_prefix_logical_not(const MathValue& value)
{
    return MathValue(builtin_truthy(value) ? 0 : 1);
}

inline MathValue builtin_prefix_abs(const MathValue& value)
{
    return value.visit([](const auto& current) {
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

inline MathValue builtin_prefix_sqrt(const MathValue& value)
{
    return MathValue(std::sqrt(value.to_double()));
}

constexpr MathValue builtin_infix_add(const MathValue& left, const MathValue& right)
{
    return left + right;
}

constexpr MathValue builtin_infix_subtract(const MathValue& left, const MathValue& right)
{
    return left - right;
}

constexpr MathValue builtin_infix_multiply(const MathValue& left, const MathValue& right)
{
    return left * right;
}

constexpr MathValue builtin_infix_divide(const MathValue& left, const MathValue& right)
{
    return left / right;
}

constexpr MathValue builtin_infix_modulo(const MathValue& left, const MathValue& right)
{
    return left % right;
}

inline MathValue builtin_infix_power(const MathValue& left, const MathValue& right)
{
    return MathValue(std::pow(left.to_double(), right.to_double()));
}

inline MathValue builtin_infix_log(const MathValue& left, const MathValue& right)
{
    return MathValue(std::log(right.to_double()) / std::log(left.to_double()));
}

constexpr MathValue builtin_infix_max(const MathValue& left, const MathValue& right)
{
    return left > right ? left : right;
}

constexpr MathValue builtin_infix_min(const MathValue& left, const MathValue& right)
{
    return left < right ? left : right;
}

constexpr MathValue builtin_infix_equal(const MathValue& left, const MathValue& right)
{
    return MathValue(left == right ? 1 : 0);
}

constexpr MathValue builtin_infix_not_equal(const MathValue& left, const MathValue& right)
{
    return MathValue(left != right ? 1 : 0);
}

constexpr MathValue builtin_infix_less(const MathValue& left, const MathValue& right)
{
    return MathValue(left < right ? 1 : 0);
}

constexpr MathValue builtin_infix_less_equal(const MathValue& left, const MathValue& right)
{
    return MathValue(left <= right ? 1 : 0);
}

constexpr MathValue builtin_infix_greater(const MathValue& left, const MathValue& right)
{
    return MathValue(left > right ? 1 : 0);
}

constexpr MathValue builtin_infix_greater_equal(const MathValue& left, const MathValue& right)
{
    return MathValue(left >= right ? 1 : 0);
}

constexpr MathValue builtin_infix_logical_and(const MathValue& left, const MathValue& right)
{
    return MathValue(builtin_truthy(left) && builtin_truthy(right) ? 1 : 0);
}

constexpr MathValue builtin_infix_logical_or(const MathValue& left, const MathValue& right)
{
    return MathValue(builtin_truthy(left) || builtin_truthy(right) ? 1 : 0);
}

inline MathValue builtin_postfix_factorial(const MathValue& value)
{
    return value.visit([](const auto& current) {
        using value_t = std::decay_t<decltype(current)>;
        if constexpr (std::is_floating_point_v<value_t>)
        {
            return MathValue(detail::factorial_value(current));
        }
        else
        {
            return MathValue(static_cast<value_t>(detail::factorial_value(static_cast<long double>(current))));
        }
    });
}

inline MathValue builtin_function_sin(const MathValue* arguments, std::size_t)
{
    return MathValue(std::sin(arguments[0].to_double()));
}

inline MathValue builtin_function_cos(const MathValue* arguments, std::size_t)
{
    return MathValue(std::cos(arguments[0].to_double()));
}

inline MathValue builtin_function_tan(const MathValue* arguments, std::size_t)
{
    return MathValue(std::tan(arguments[0].to_double()));
}

inline MathValue builtin_function_asin(const MathValue* arguments, std::size_t)
{
    return MathValue(std::asin(arguments[0].to_double()));
}

inline MathValue builtin_function_acos(const MathValue* arguments, std::size_t)
{
    return MathValue(std::acos(arguments[0].to_double()));
}

inline MathValue builtin_function_atan(const MathValue* arguments, std::size_t)
{
    return MathValue(std::atan(arguments[0].to_double()));
}

inline MathValue builtin_function_sqrt(const MathValue* arguments, std::size_t)
{
    return MathValue(std::sqrt(arguments[0].to_double()));
}

inline MathValue builtin_function_ceil(const MathValue* arguments, std::size_t)
{
    return MathValue(std::ceil(arguments[0].to_double()));
}

inline MathValue builtin_function_floor(const MathValue* arguments, std::size_t)
{
    return MathValue(std::floor(arguments[0].to_double()));
}

inline MathValue builtin_function_round(const MathValue* arguments, std::size_t)
{
    return MathValue(std::round(arguments[0].to_double()));
}

inline MathValue builtin_function_exp(const MathValue* arguments, std::size_t)
{
    return MathValue(std::exp(arguments[0].to_double()));
}

inline MathValue builtin_function_abs(const MathValue* arguments, std::size_t)
{
    return builtin_prefix_abs(arguments[0]);
}

inline MathValue builtin_function_rad(const MathValue* arguments, std::size_t)
{
    return MathValue(arguments[0].to_double() * (3.14159265358979323846 / 180.0));
}

inline MathValue builtin_function_deg(const MathValue* arguments, std::size_t)
{
    return MathValue(arguments[0].to_double() * (180.0 / 3.14159265358979323846));
}

constexpr MathValue builtin_function_max(const MathValue* arguments, std::size_t count)
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

constexpr MathValue builtin_function_min(const MathValue* arguments, std::size_t count)
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

inline constexpr std::array<BuiltinPrefixOperatorSpec, 5> k_builtin_prefix_operators{{
    {"+", Precedence::Prefix, OperatorSemantics::identity(), &builtin_prefix_identity},
    {"-", Precedence::Prefix, OperatorSemantics::negate(), &builtin_prefix_negate},
    {"!", Precedence::Prefix, OperatorSemantics::logical_not_op(), &builtin_prefix_logical_not},
    {"abs", Precedence::Prefix, FunctionSemantics::pure(), &builtin_prefix_abs},
    {"sqrt", Precedence::Prefix, FunctionSemantics::pure(), &builtin_prefix_sqrt},
}};

inline constexpr std::array<BuiltinPostfixOperatorSpec, 1> k_builtin_postfix_operators{{
    {"!", Precedence::Postfix, OperatorSemantics::factorial(), &builtin_postfix_factorial},
}};

inline constexpr std::array<BuiltinInfixOperatorSpec, 17> k_builtin_infix_operators{{
    {"+", Precedence::Additive, Associativity::Left, OperatorSemantics::arithmetic_add(), &builtin_infix_add},
    {"-", Precedence::Additive, Associativity::Left, OperatorSemantics::arithmetic_subtract(), &builtin_infix_subtract},
    {"*", Precedence::Multiplicative, Associativity::Left, OperatorSemantics::arithmetic_multiply(), &builtin_infix_multiply},
    {"/", Precedence::Multiplicative, Associativity::Left, OperatorSemantics::arithmetic_divide(), &builtin_infix_divide},
    {"%", Precedence::Multiplicative, Associativity::Left, OperatorSemantics::arithmetic_modulo(), &builtin_infix_modulo},
    {"^", Precedence::Power, Associativity::Right, OperatorSemantics::power(), &builtin_infix_power},
    {"log", Precedence::Power, Associativity::Left, OperatorSemantics::logarithm(), &builtin_infix_log},
    {"==", Precedence::Comparison, Associativity::Left, OperatorSemantics::comparison(ArithmeticKind::CompareEqual), &builtin_infix_equal},
    {"!=", Precedence::Comparison, Associativity::Left, OperatorSemantics::comparison(ArithmeticKind::CompareNotEqual), &builtin_infix_not_equal},
    {"<", Precedence::Comparison, Associativity::Left, OperatorSemantics::comparison(ArithmeticKind::CompareLess), &builtin_infix_less},
    {"<=", Precedence::Comparison, Associativity::Left, OperatorSemantics::comparison(ArithmeticKind::CompareLessEqual), &builtin_infix_less_equal},
    {">", Precedence::Comparison, Associativity::Left, OperatorSemantics::comparison(ArithmeticKind::CompareGreater), &builtin_infix_greater},
    {">=", Precedence::Comparison, Associativity::Left, OperatorSemantics::comparison(ArithmeticKind::CompareGreaterEqual), &builtin_infix_greater_equal},
    {"&&", Precedence::LogicalAnd, Associativity::Left, OperatorSemantics::logical_and_op(), &builtin_infix_logical_and},
    {"||", Precedence::LogicalOr, Associativity::Left, OperatorSemantics::logical_or_op(), &builtin_infix_logical_or},
    {"max", Precedence::Power, Associativity::Left, OperatorSemantics::max_op(), &builtin_infix_max},
    {"min", Precedence::Power, Associativity::Left, OperatorSemantics::min_op(), &builtin_infix_min},
}};

inline constexpr std::array<BuiltinFunctionSpec, 16> k_builtin_functions{{
    {"sin", FunctionSemantics::pure(), &builtin_function_sin},
    {"cos", FunctionSemantics::pure(), &builtin_function_cos},
    {"tan", FunctionSemantics::pure(), &builtin_function_tan},
    {"asin", FunctionSemantics::pure(), &builtin_function_asin},
    {"acos", FunctionSemantics::pure(), &builtin_function_acos},
    {"atan", FunctionSemantics::pure(), &builtin_function_atan},
    {"sqrt", FunctionSemantics::pure(), &builtin_function_sqrt},
    {"ceil", FunctionSemantics::pure(), &builtin_function_ceil},
    {"floor", FunctionSemantics::pure(), &builtin_function_floor},
    {"round", FunctionSemantics::pure(), &builtin_function_round},
    {"exp", FunctionSemantics::pure(), &builtin_function_exp},
    {"abs", FunctionSemantics::pure(), &builtin_function_abs},
    {"rad", FunctionSemantics::pure(), &builtin_function_rad},
    {"deg", FunctionSemantics::pure(), &builtin_function_deg},
    {"max", FunctionSemantics::pure(), &builtin_function_max},
    {"min", FunctionSemantics::pure(), &builtin_function_min},
}};

template <class Spec, std::size_t Count>
constexpr const Spec* find_builtin_spec(const std::array<Spec, Count>& specs, std::string_view name) noexcept
{
    for (const auto& spec : specs)
    {
        if (spec.symbol == name)
        {
            return &spec;
        }
    }
    return nullptr;
}

template <std::size_t Count>
constexpr const BuiltinFunctionSpec* find_builtin_function_spec(
    const std::array<BuiltinFunctionSpec, Count>& specs,
    std::string_view name) noexcept
{
    for (const auto& spec : specs)
    {
        if (spec.name == name)
        {
            return &spec;
        }
    }
    return nullptr;
}

template <class Spec, std::size_t Count>
constexpr std::string_view match_builtin_symbol_impl(
    const std::array<Spec, Count>& specs,
    std::string_view text) noexcept
{
    std::string_view best;
    for (const auto& spec : specs)
    {
        const auto symbol = spec.symbol;
        if (text.size() >= symbol.size() && text.substr(0, symbol.size()) == symbol && symbol.size() > best.size())
        {
            best = symbol;
        }
    }
    return best;
}

constexpr const BuiltinPrefixOperatorSpec* find_builtin_prefix_operator(std::string_view symbol) noexcept
{
    return find_builtin_spec(k_builtin_prefix_operators, symbol);
}

constexpr const BuiltinPostfixOperatorSpec* find_builtin_postfix_operator(std::string_view symbol) noexcept
{
    return find_builtin_spec(k_builtin_postfix_operators, symbol);
}

constexpr const BuiltinInfixOperatorSpec* find_builtin_infix_operator(std::string_view symbol) noexcept
{
    return find_builtin_spec(k_builtin_infix_operators, symbol);
}

constexpr const BuiltinFunctionSpec* find_builtin_function(std::string_view name) noexcept
{
    return find_builtin_function_spec(k_builtin_functions, name);
}

constexpr std::string_view match_builtin_prefix_symbol(std::string_view text) noexcept
{
    return match_builtin_symbol_impl(k_builtin_prefix_operators, text);
}

constexpr std::string_view match_builtin_postfix_symbol(std::string_view text) noexcept
{
    return match_builtin_symbol_impl(k_builtin_postfix_operators, text);
}

constexpr std::string_view match_builtin_infix_symbol(std::string_view text) noexcept
{
    return match_builtin_symbol_impl(k_builtin_infix_operators, text);
}

inline const BuiltinPrefixOperatorSpec& require_builtin_prefix_spec(std::string_view symbol)
{
    const auto* spec = find_builtin_prefix_operator(symbol);
    if (spec == nullptr)
    {
        throw std::logic_error("string_math: unknown builtin prefix operator");
    }
    return *spec;
}

inline const BuiltinPostfixOperatorSpec& require_builtin_postfix_spec(std::string_view symbol)
{
    const auto* spec = find_builtin_postfix_operator(symbol);
    if (spec == nullptr)
    {
        throw std::logic_error("string_math: unknown builtin postfix operator");
    }
    return *spec;
}

inline const BuiltinInfixOperatorSpec& require_builtin_infix_spec(std::string_view symbol)
{
    const auto* spec = find_builtin_infix_operator(symbol);
    if (spec == nullptr)
    {
        throw std::logic_error("string_math: unknown builtin infix operator");
    }
    return *spec;
}

inline const BuiltinFunctionSpec& require_builtin_function_spec(std::string_view name)
{
    const auto* spec = find_builtin_function(name);
    if (spec == nullptr)
    {
        throw std::logic_error("string_math: unknown builtin function");
    }
    return *spec;
}

template <class... Ts, class F>
void register_builtin_infix(
    MathContext& context,
    std::string_view symbol,
    F&& function,
    CallableSemantics semantics = {})
{
    const auto& spec = require_builtin_infix_spec(symbol);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    context.add_infix_operator_for<Ts...>(
        std::string(symbol),
        std::forward<F>(function),
        spec.precedence,
        spec.associativity,
        effective_semantics);
}

template <class... Ts, class F>
void register_builtin_prefix(
    MathContext& context,
    std::string_view symbol,
    F&& function,
    CallableSemantics semantics = {})
{
    const auto& spec = require_builtin_prefix_spec(symbol);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    context.add_prefix_operator_for<Ts...>(
        std::string(symbol),
        std::forward<F>(function),
        spec.precedence,
        effective_semantics);
}

template <class... Ts, class F>
void register_builtin_postfix(
    MathContext& context,
    std::string_view symbol,
    F&& function,
    CallableSemantics semantics = {})
{
    const auto& spec = require_builtin_postfix_spec(symbol);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    context.add_postfix_operator_for<Ts...>(
        std::string(symbol),
        std::forward<F>(function),
        spec.precedence,
        effective_semantics);
}

template <class... Ts, class F>
void register_builtin_function(MathContext& context, std::string_view name, F&& function, CallableSemantics semantics = {})
{
    const auto& spec = require_builtin_function_spec(name);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    context.add_function_for<Ts...>(std::string(name), std::forward<F>(function), effective_semantics);
}

} // namespace detail

inline MathContext MathContext::with_builtins()
{
    MathContext context;

    detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        context, "+", [](auto left, auto right) { return left + right; });
    detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        context, "-", [](auto left, auto right) { return left - right; });
    detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        context, "*", [](auto left, auto right) { return left * right; });

    detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        context,
        "/",
        [](auto left, auto right) {
            if (right == 0)
            {
                throw std::domain_error("string_math: division by zero");
            }
            return left / right;
        });

    detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        context,
        "%",
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
        });

    detail::register_builtin_infix<double, long double>(
        context, "^", [](auto left, auto right) { return std::pow(left, right); });
    detail::register_builtin_infix<double, long double>(
        context, "log", [](auto base, auto value) { return std::log(value) / std::log(base); });
    detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        context, "==", [](auto left, auto right) { return left == right ? 1 : 0; });
    detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        context, "!=", [](auto left, auto right) { return left != right ? 1 : 0; });
    detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        context, "<", [](auto left, auto right) { return left < right ? 1 : 0; });
    detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        context, "<=", [](auto left, auto right) { return left <= right ? 1 : 0; });
    detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        context, ">", [](auto left, auto right) { return left > right ? 1 : 0; });
    detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        context, ">=", [](auto left, auto right) { return left >= right ? 1 : 0; });
    detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        context, "&&", [](auto left, auto right) { return (left != 0 && right != 0) ? 1 : 0; });
    detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        context, "||", [](auto left, auto right) { return (left != 0 || right != 0) ? 1 : 0; });
    detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        context, "max", [](auto left, auto right) { return std::max(left, right); });
    detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        context, "min", [](auto left, auto right) { return std::min(left, right); });

    detail::register_builtin_prefix<STRING_MATH_ALL_VALUE_TYPES>(context, "+", [](auto value) { return value; });
    detail::register_builtin_prefix<STRING_MATH_SIGNED_AND_FLOATING_VALUE_TYPES>(context, "-", [](auto value) {
        return -value;
    });
    detail::register_builtin_prefix<STRING_MATH_ALL_VALUE_TYPES>(
        context, "!", [](auto value) { return value == 0 ? 1 : 0; });
    context.add_postfix_operator_overloads<STRING_MATH_SIGNED_FACTORIAL_SIGNATURES, STRING_MATH_UNSIGNED_FACTORIAL_SIGNATURES>(
        "!", [](auto value) {
        using value_t = std::decay_t<decltype(value)>;
        return static_cast<value_t>(detail::factorial_value(static_cast<long double>(value)));
        }, detail::require_builtin_postfix_spec("!").precedence, detail::require_builtin_postfix_spec("!").semantics);
    detail::register_builtin_postfix<double, long double>(
        context, "!", [](auto value) { return detail::factorial_value(value); });

    detail::register_builtin_function<float, double, long double>(context, "sin", [](auto value) { return std::sin(value); });
    detail::register_builtin_function<float, double, long double>(context, "cos", [](auto value) { return std::cos(value); });
    detail::register_builtin_function<float, double, long double>(context, "tan", [](auto value) { return std::tan(value); });
    detail::register_builtin_function<float, double, long double>(context, "asin", [](auto value) { return std::asin(value); });
    detail::register_builtin_function<float, double, long double>(context, "acos", [](auto value) { return std::acos(value); });
    detail::register_builtin_function<float, double, long double>(context, "atan", [](auto value) { return std::atan(value); });
    detail::register_builtin_function<float, double, long double>(context, "sqrt", [](auto value) { return std::sqrt(value); });
    detail::register_builtin_function<float, double, long double>(context, "ceil", [](auto value) { return std::ceil(value); });
    detail::register_builtin_function<float, double, long double>(context, "floor", [](auto value) { return std::floor(value); });
    detail::register_builtin_function<float, double, long double>(context, "round", [](auto value) { return std::round(value); });
    detail::register_builtin_function<float, double, long double>(context, "exp", [](auto value) { return std::exp(value); });
    detail::register_builtin_function<STRING_MATH_SIGNED_AND_FLOATING_VALUE_TYPES>(context, "abs", [](auto value) {
        return std::abs(value);
    });
    context.add_function(
        "rad",
        [](double value) { return value * (3.14159265358979323846 / 180.0); },
        detail::require_builtin_function_spec("rad").semantics);
    context.add_function(
        "deg",
        [](double value) { return value * (180.0 / 3.14159265358979323846); },
        detail::require_builtin_function_spec("deg").semantics);
    context.add_variadic_function("max", 1, [](const std::vector<MathValue>& arguments) {
        MathValue current = arguments.front();
        for (std::size_t index = 1; index < arguments.size(); ++index)
        {
            if (arguments[index] > current)
            {
                current = arguments[index];
            }
        }
        return current;
    }, detail::require_builtin_function_spec("max").semantics);
    context.add_variadic_function("min", 1, [](const std::vector<MathValue>& arguments) {
        MathValue current = arguments.front();
        for (std::size_t index = 1; index < arguments.size(); ++index)
        {
            if (arguments[index] < current)
            {
                current = arguments[index];
            }
        }
        return current;
    }, detail::require_builtin_function_spec("min").semantics);

    detail::register_builtin_prefix<STRING_MATH_SIGNED_AND_FLOATING_VALUE_TYPES>(context, "abs", [](auto value) {
        return std::abs(value);
    });
    detail::register_builtin_prefix<float, double, long double>(
        context, "sqrt", [](auto value) { return std::sqrt(value); });

    context.set_value("PI", MathValue(3.14159265358979323846));
    context.set_value("E", MathValue(2.71828182845904523536));

    return context;
}

} // namespace string_math
