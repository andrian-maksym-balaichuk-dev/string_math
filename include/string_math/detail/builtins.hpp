#pragma once

#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <string_math/detail/builtin_specs.hpp>
#include <string_math/context.hpp>

namespace string_math
{

namespace detail
{

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
template <
    std::size_t VariableCount,
    std::size_t InfixCount,
    std::size_t FunctionCount,
    std::size_t PrefixCount,
    std::size_t PostfixCount>
class StaticMathContext;
#endif

template <class... Ts, class F>
MathContext register_builtin_infix(
    MathContext context,
    std::string_view symbol,
    F&& function,
    CallableSemantics semantics = {})
{
    const auto spec = require_builtin_infix_spec(symbol);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    context.add_infix_operator_for<Ts...>(
        std::string(symbol), std::forward<F>(function), spec.precedence, spec.associativity, effective_semantics);
    return context;
}

template <class... Ts, class F>
MathContext register_builtin_prefix(
    MathContext context,
    std::string_view symbol,
    F&& function,
    CallableSemantics semantics = {})
{
    const auto spec = require_builtin_prefix_spec(symbol);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    context.add_prefix_operator_for<Ts...>(
        std::string(symbol), std::forward<F>(function), spec.precedence, effective_semantics);
    return context;
}

template <class... Ts, class F>
MathContext register_builtin_postfix(
    MathContext context,
    std::string_view symbol,
    F&& function,
    CallableSemantics semantics = {})
{
    const auto spec = require_builtin_postfix_spec(symbol);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    context.add_postfix_operator_for<Ts...>(
        std::string(symbol), std::forward<F>(function), spec.precedence, effective_semantics);
    return context;
}

template <class... Ts, class F>
MathContext register_builtin_function(
    MathContext context,
    std::string_view name,
    F&& function,
    CallableSemantics semantics = {})
{
    const auto spec = require_builtin_function_spec(name);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    context.add_function_for<Ts...>(std::string(name), std::forward<F>(function), effective_semantics);
    return context;
}

template <class Sig, class F>
MathContext register_builtin_function_signature(
    MathContext context,
    std::string_view name,
    F&& function,
    CallableSemantics semantics = {})
{
    const auto spec = require_builtin_function_spec(name);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    context.template add_function<Sig>(std::string(name), std::forward<F>(function), effective_semantics);
    return context;
}

template <class... Sigs, class F>
MathContext register_builtin_postfix_overloads(
    MathContext context,
    std::string_view symbol,
    F&& function,
    CallableSemantics semantics = {})
{
    const auto spec = require_builtin_postfix_spec(symbol);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    context.template add_postfix_operator_overloads<Sigs...>(
        std::string(symbol), std::forward<F>(function), spec.precedence, effective_semantics);
    return context;
}

template <class F>
MathContext register_builtin_variadic_function(
    MathContext context,
    std::string_view name,
    std::size_t min_arity,
    F&& function,
    CallableSemantics semantics = {})
{
    const auto spec = require_builtin_function_spec(name);
    const CallableSemantics effective_semantics = semantics.empty() ? spec.semantics : semantics;
    context.add_variadic_function(std::string(name), min_arity, std::forward<F>(function), effective_semantics);
    return context;
}

inline MathContext register_builtin_value(MathContext context, std::string_view name, MathValue value)
{
    context.set_value(std::string(name), value);
    return context;
}

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
template <class... Ts, std::size_t VariableCount, std::size_t InfixCount, std::size_t FunctionCount, std::size_t PrefixCount, std::size_t PostfixCount, class F>
constexpr auto register_builtin_infix(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view symbol,
    F&& function,
    CallableSemantics semantics = {});

template <class... Ts, std::size_t VariableCount, std::size_t InfixCount, std::size_t FunctionCount, std::size_t PrefixCount, std::size_t PostfixCount, class F>
constexpr auto register_builtin_prefix(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view symbol,
    F&& function,
    CallableSemantics semantics = {});

template <class... Ts, std::size_t VariableCount, std::size_t InfixCount, std::size_t FunctionCount, std::size_t PrefixCount, std::size_t PostfixCount, class F>
constexpr auto register_builtin_postfix(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view symbol,
    F&& function,
    CallableSemantics semantics = {});

template <class... Ts, std::size_t VariableCount, std::size_t InfixCount, std::size_t FunctionCount, std::size_t PrefixCount, std::size_t PostfixCount, class F>
constexpr auto register_builtin_function(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view name,
    F&& function,
    CallableSemantics semantics = {});

template <class Sig, std::size_t VariableCount, std::size_t InfixCount, std::size_t FunctionCount, std::size_t PrefixCount, std::size_t PostfixCount, class F>
constexpr auto register_builtin_function_signature(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view name,
    F&& function,
    CallableSemantics semantics = {});

template <class... Sigs, std::size_t VariableCount, std::size_t InfixCount, std::size_t FunctionCount, std::size_t PrefixCount, std::size_t PostfixCount, class F>
constexpr auto register_builtin_postfix_overloads(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view symbol,
    F&& function,
    CallableSemantics semantics = {});

template <std::size_t VariableCount, std::size_t InfixCount, std::size_t FunctionCount, std::size_t PrefixCount, std::size_t PostfixCount, class F>
constexpr auto register_builtin_variadic_function(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view name,
    std::size_t min_arity,
    F&& function,
    CallableSemantics semantics = {});

template <std::size_t VariableCount, std::size_t InfixCount, std::size_t FunctionCount, std::size_t PrefixCount, std::size_t PostfixCount>
constexpr auto register_builtin_value(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view name,
    MathValue value);
#endif

} // namespace detail

template <class Context>
constexpr auto add_builtin_constants(Context context)
{
    const auto c01 = detail::register_builtin_value(std::move(context), "PI", MathValue(3.14159265358979323846));
    return detail::register_builtin_value(std::move(c01), "E", MathValue(2.71828182845904523536));
}

template <class Context = MathContext>
constexpr auto with_builtins(Context context = {})
{
    const auto c01 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(context), "+", [](auto left, auto right) { return left + right; });
    const auto c02 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c01), "-", [](auto left, auto right) { return left - right; });
    const auto c03 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c02), "*", [](auto left, auto right) { return left * right; });

    const auto c04 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c03),
        "/",
        [](auto left, auto right) {
            if (right == 0)
            {
                throw std::domain_error("string_math: division by zero");
            }
            return left / right;
        });

    const auto c05 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c04),
        "%",
        [](auto left, auto right) {
            if (right == 0)
            {
                throw std::domain_error("string_math: modulo by zero");
            }
            using value_t = std::decay_t<decltype(left)>;
            if constexpr (std::is_integral_v<value_t>)
                return left % right;
            else
                return std::fmod(left, right);
        });

    const auto c06 = detail::register_builtin_infix<double, long double>(
        std::move(c05), "^", [](auto left, auto right) { return std::pow(left, right); });
    const auto c07 = detail::register_builtin_infix<double, long double>(
        std::move(c06), "log", [](auto base, auto value) { return std::log(value) / std::log(base); });
    const auto c08 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c07), "==", [](auto left, auto right) { return left == right ? 1 : 0; });
    const auto c09 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c08), "!=", [](auto left, auto right) { return left != right ? 1 : 0; });
    const auto c10 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c09), "<", [](auto left, auto right) { return left < right ? 1 : 0; });
    const auto c11 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c10), "<=", [](auto left, auto right) { return left <= right ? 1 : 0; });
    const auto c12 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c11), ">", [](auto left, auto right) { return left > right ? 1 : 0; });
    const auto c13 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c12), ">=", [](auto left, auto right) { return left >= right ? 1 : 0; });
    const auto c14 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c13), "&&", [](auto left, auto right) { return (left != 0 && right != 0) ? 1 : 0; });
    const auto c15 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c14), "||", [](auto left, auto right) { return (left != 0 || right != 0) ? 1 : 0; });
    const auto c16 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c15), "max", [](auto left, auto right) { return std::max(left, right); });
    const auto c17 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c16), "min", [](auto left, auto right) { return std::min(left, right); });

    const auto c18 = detail::register_builtin_prefix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c17), "+", [](auto value) { return value; });
    const auto c19 = detail::register_builtin_prefix<STRING_MATH_SIGNED_AND_FLOATING_VALUE_TYPES>(
        std::move(c18), "-", [](auto value) { return -value; });
    const auto c20 = detail::register_builtin_prefix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c19), "!", [](auto value) { return value == 0 ? 1 : 0; });
    const auto c21 = detail::register_builtin_postfix_overloads<STRING_MATH_SIGNED_FACTORIAL_SIGNATURES, STRING_MATH_UNSIGNED_FACTORIAL_SIGNATURES>(
        std::move(c20),
        "!",
        [](auto value) {
            using value_t = std::decay_t<decltype(value)>;
            return static_cast<value_t>(detail::factorial_value(static_cast<long double>(value)));
        });
    const auto c22 = detail::register_builtin_postfix<double, long double>(
        std::move(c21), "!", [](auto value) { return detail::factorial_value(value); });

    const auto c23 = detail::register_builtin_function<float, double, long double>(
        std::move(c22), "sin", [](auto value) { return std::sin(value); });
    const auto c24 = detail::register_builtin_function<float, double, long double>(
        std::move(c23), "cos", [](auto value) { return std::cos(value); });
    const auto c25 = detail::register_builtin_function<float, double, long double>(
        std::move(c24), "tan", [](auto value) { return std::tan(value); });
    const auto c26 = detail::register_builtin_function<float, double, long double>(
        std::move(c25), "asin", [](auto value) { return std::asin(value); });
    const auto c27 = detail::register_builtin_function<float, double, long double>(
        std::move(c26), "acos", [](auto value) { return std::acos(value); });
    const auto c28 = detail::register_builtin_function<float, double, long double>(
        std::move(c27), "atan", [](auto value) { return std::atan(value); });
    const auto c29 = detail::register_builtin_function<float, double, long double>(
        std::move(c28), "sqrt", [](auto value) { return std::sqrt(value); });
    const auto c30 = detail::register_builtin_function<float, double, long double>(
        std::move(c29), "ceil", [](auto value) { return std::ceil(value); });
    const auto c31 = detail::register_builtin_function<float, double, long double>(
        std::move(c30), "floor", [](auto value) { return std::floor(value); });
    const auto c32 = detail::register_builtin_function<float, double, long double>(
        std::move(c31), "round", [](auto value) { return std::round(value); });
    const auto c33 = detail::register_builtin_function<float, double, long double>(
        std::move(c32), "exp", [](auto value) { return std::exp(value); });
    const auto c34 = detail::register_builtin_function<STRING_MATH_SIGNED_AND_FLOATING_VALUE_TYPES>(
        std::move(c33), "abs", [](auto value) { return std::abs(value); });
    const auto c35 = detail::register_builtin_function_signature<double(double)>(
        std::move(c34),
        "rad",
        [](double value) { return value * (3.14159265358979323846 / 180.0); });
    const auto c36 = detail::register_builtin_function_signature<double(double)>(
        std::move(c35),
        "deg",
        [](double value) { return value * (180.0 / 3.14159265358979323846); });
    const auto c37 = detail::register_builtin_variadic_function(
        std::move(c36), "max", 1, [](const std::vector<MathValue>& arguments) {
            MathValue current = arguments.front();
            for (std::size_t index = 1; index < arguments.size(); ++index)
            {
                if (arguments[index] > current)
                {
                    current = arguments[index];
                }
            }
            return current;
        });
    const auto c38 = detail::register_builtin_variadic_function(
        std::move(c37), "min", 1, [](const std::vector<MathValue>& arguments) {
            MathValue current = arguments.front();
            for (std::size_t index = 1; index < arguments.size(); ++index)
            {
                if (arguments[index] < current)
                {
                    current = arguments[index];
                }
            }
            return current;
        });

    const auto c39 = detail::register_builtin_prefix<STRING_MATH_SIGNED_AND_FLOATING_VALUE_TYPES>(
        std::move(c38), "abs", [](auto value) { return std::abs(value); });
    const auto c40 = detail::register_builtin_prefix<float, double, long double>(
        std::move(c39), "sqrt", [](auto value) { return std::sqrt(value); });
    return add_builtin_constants(std::move(c40));
}

inline MathContext MathContext::with_builtins()
{
    return string_math::with_builtins(MathContext{});
}

} // namespace string_math
