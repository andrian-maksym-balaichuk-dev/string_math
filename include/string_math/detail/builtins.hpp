#pragma once

#include <string_math/context.hpp>

namespace string_math
{

inline MathContext MathContext::with_builtins()
{
    MathContext context;

    context.add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
        "+", [](auto left, auto right) { return left + right; }, Precedence::Additive);
    context.add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
        "-", [](auto left, auto right) { return left - right; }, Precedence::Additive);
    context.add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
        "*", [](auto left, auto right) { return left * right; }, Precedence::Multiplicative);

    context.add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
        "/",
        [](auto left, auto right) {
            if (right == 0)
            {
                throw std::domain_error("string_math: division by zero");
            }
            return left / right;
        },
        Precedence::Multiplicative);

    context.add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
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
        },
        Precedence::Multiplicative);

    context.add_infix_operator_for<double, long double>(
        "^", [](auto left, auto right) { return std::pow(left, right); }, Precedence::Power, Associativity::Right);
    context.add_infix_operator_for<double, long double>(
        "log", [](auto base, auto value) { return std::log(value) / std::log(base); }, Precedence::Power, Associativity::Left);
    context.add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
        "==", [](auto left, auto right) { return left == right ? 1 : 0; }, Precedence::Comparison, Associativity::Left);
    context.add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
        "!=", [](auto left, auto right) { return left != right ? 1 : 0; }, Precedence::Comparison, Associativity::Left);
    context.add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
        "<", [](auto left, auto right) { return left < right ? 1 : 0; }, Precedence::Comparison, Associativity::Left);
    context.add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
        "<=", [](auto left, auto right) { return left <= right ? 1 : 0; }, Precedence::Comparison, Associativity::Left);
    context.add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
        ">", [](auto left, auto right) { return left > right ? 1 : 0; }, Precedence::Comparison, Associativity::Left);
    context.add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
        ">=", [](auto left, auto right) { return left >= right ? 1 : 0; }, Precedence::Comparison, Associativity::Left);
    context.add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
        "&&", [](auto left, auto right) { return (left != 0 && right != 0) ? 1 : 0; }, Precedence::LogicalAnd, Associativity::Left);
    context.add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
        "||", [](auto left, auto right) { return (left != 0 || right != 0) ? 1 : 0; }, Precedence::LogicalOr, Associativity::Left);
    context.add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
        "max", [](auto left, auto right) { return std::max(left, right); }, Precedence::Power, Associativity::Left);
    context.add_infix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
        "min", [](auto left, auto right) { return std::min(left, right); }, Precedence::Power, Associativity::Left);

    context.add_prefix_operator_for<STRING_MATH_ALL_VALUE_TYPES>(
        "+", [](auto value) { return value; });
    context.add_prefix_operator_for<STRING_MATH_SIGNED_AND_FLOATING_VALUE_TYPES>("-", [](auto value) {
        return -value;
    });
    context.add_prefix_operator_for<STRING_MATH_ALL_VALUE_TYPES>("!", [](auto value) { return value == 0 ? 1 : 0; });
    context.add_postfix_operator_overloads<STRING_MATH_SIGNED_FACTORIAL_SIGNATURES, STRING_MATH_UNSIGNED_FACTORIAL_SIGNATURES>("!", [](auto value) {
        using value_t = std::decay_t<decltype(value)>;
        return static_cast<value_t>(detail::factorial_value(static_cast<long double>(value)));
    });
    context.add_postfix_operator_for<double, long double>("!", [](auto value) { return detail::factorial_value(value); });

    context.add_function_for<float, double, long double>("sin", [](auto value) { return std::sin(value); });
    context.add_function_for<float, double, long double>("cos", [](auto value) { return std::cos(value); });
    context.add_function_for<float, double, long double>("tan", [](auto value) { return std::tan(value); });
    context.add_function_for<float, double, long double>("asin", [](auto value) { return std::asin(value); });
    context.add_function_for<float, double, long double>("acos", [](auto value) { return std::acos(value); });
    context.add_function_for<float, double, long double>("atan", [](auto value) { return std::atan(value); });
    context.add_function_for<float, double, long double>("sqrt", [](auto value) { return std::sqrt(value); });
    context.add_function_for<float, double, long double>("ceil", [](auto value) { return std::ceil(value); });
    context.add_function_for<float, double, long double>("floor", [](auto value) { return std::floor(value); });
    context.add_function_for<float, double, long double>("round", [](auto value) { return std::round(value); });
    context.add_function_for<float, double, long double>("exp", [](auto value) { return std::exp(value); });
    context.add_function_for<STRING_MATH_SIGNED_AND_FLOATING_VALUE_TYPES>("abs", [](auto value) {
        return std::abs(value);
    });
    context.add_function("rad", [](double value) { return value * (3.14159265358979323846 / 180.0); });
    context.add_function("deg", [](double value) { return value * (180.0 / 3.14159265358979323846); });
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
    });
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
    });

    context.add_prefix_operator_for<STRING_MATH_SIGNED_AND_FLOATING_VALUE_TYPES>("abs", [](auto value) {
        return std::abs(value);
    });
    context.add_prefix_operator_for<float, double, long double>("sqrt", [](auto value) { return std::sqrt(value); });

    context.set_value("PI", MathValue(3.14159265358979323846));
    context.set_value("E", MathValue(2.71828182845904523536));

    return context;
}

} // namespace string_math
