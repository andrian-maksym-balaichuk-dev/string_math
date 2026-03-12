#pragma once

#include <optional>
#include <stdexcept>
#include <string_view>

#include <string_math/detail/builtin_functions.hpp>

namespace string_math::detail
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

constexpr std::optional<BuiltinPrefixOperatorSpec> find_builtin_prefix_operator(std::string_view symbol) noexcept
{
    if (symbol == "+") return BuiltinPrefixOperatorSpec{"+", Precedence::Prefix, OperatorSemantics::identity(), &builtin_prefix_identity};
    if (symbol == "-") return BuiltinPrefixOperatorSpec{"-", Precedence::Prefix, OperatorSemantics::negate(), &builtin_prefix_negate};
    if (symbol == "!") return BuiltinPrefixOperatorSpec{"!", Precedence::Prefix, OperatorSemantics::logical_not_op(), &builtin_prefix_logical_not};
    if (symbol == "abs") return BuiltinPrefixOperatorSpec{"abs", Precedence::Prefix, FunctionSemantics::pure(), &builtin_prefix_abs};
    if (symbol == "sqrt") return BuiltinPrefixOperatorSpec{"sqrt", Precedence::Prefix, FunctionSemantics::pure(), &builtin_prefix_sqrt};
    return std::nullopt;
}

constexpr std::optional<BuiltinPostfixOperatorSpec> find_builtin_postfix_operator(std::string_view symbol) noexcept
{
    if (symbol == "!") return BuiltinPostfixOperatorSpec{"!", Precedence::Postfix, OperatorSemantics::factorial(), &builtin_postfix_factorial};
    return std::nullopt;
}

constexpr std::optional<BuiltinInfixOperatorSpec> find_builtin_infix_operator(std::string_view symbol) noexcept
{
    if (symbol == "+") return BuiltinInfixOperatorSpec{"+", Precedence::Additive, Associativity::Left, OperatorSemantics::arithmetic_add(), &builtin_infix_add};
    if (symbol == "-") return BuiltinInfixOperatorSpec{"-", Precedence::Additive, Associativity::Left, OperatorSemantics::arithmetic_subtract(), &builtin_infix_subtract};
    if (symbol == "*") return BuiltinInfixOperatorSpec{"*", Precedence::Multiplicative, Associativity::Left, OperatorSemantics::arithmetic_multiply(), &builtin_infix_multiply};
    if (symbol == "/") return BuiltinInfixOperatorSpec{"/", Precedence::Multiplicative, Associativity::Left, OperatorSemantics::arithmetic_divide(), &builtin_infix_divide};
    if (symbol == "%") return BuiltinInfixOperatorSpec{"%", Precedence::Multiplicative, Associativity::Left, OperatorSemantics::arithmetic_modulo(), &builtin_infix_modulo};
    if (symbol == "^") return BuiltinInfixOperatorSpec{"^", Precedence::Power, Associativity::Right, OperatorSemantics::power(), &builtin_infix_power};
    if (symbol == "log") return BuiltinInfixOperatorSpec{"log", Precedence::Power, Associativity::Left, OperatorSemantics::logarithm(), &builtin_infix_log};
    if (symbol == "==") return BuiltinInfixOperatorSpec{"==", Precedence::Comparison, Associativity::Left, OperatorSemantics::comparison(ArithmeticKind::CompareEqual), &builtin_infix_equal};
    if (symbol == "!=") return BuiltinInfixOperatorSpec{"!=", Precedence::Comparison, Associativity::Left, OperatorSemantics::comparison(ArithmeticKind::CompareNotEqual), &builtin_infix_not_equal};
    if (symbol == "<") return BuiltinInfixOperatorSpec{"<", Precedence::Comparison, Associativity::Left, OperatorSemantics::comparison(ArithmeticKind::CompareLess), &builtin_infix_less};
    if (symbol == "<=") return BuiltinInfixOperatorSpec{"<=", Precedence::Comparison, Associativity::Left, OperatorSemantics::comparison(ArithmeticKind::CompareLessEqual), &builtin_infix_less_equal};
    if (symbol == ">") return BuiltinInfixOperatorSpec{">", Precedence::Comparison, Associativity::Left, OperatorSemantics::comparison(ArithmeticKind::CompareGreater), &builtin_infix_greater};
    if (symbol == ">=") return BuiltinInfixOperatorSpec{">=", Precedence::Comparison, Associativity::Left, OperatorSemantics::comparison(ArithmeticKind::CompareGreaterEqual), &builtin_infix_greater_equal};
    if (symbol == "&&") return BuiltinInfixOperatorSpec{"&&", Precedence::LogicalAnd, Associativity::Left, OperatorSemantics::logical_and_op(), &builtin_infix_logical_and};
    if (symbol == "||") return BuiltinInfixOperatorSpec{"||", Precedence::LogicalOr, Associativity::Left, OperatorSemantics::logical_or_op(), &builtin_infix_logical_or};
    if (symbol == "max") return BuiltinInfixOperatorSpec{"max", Precedence::Power, Associativity::Left, OperatorSemantics::max_op(), &builtin_infix_max};
    if (symbol == "min") return BuiltinInfixOperatorSpec{"min", Precedence::Power, Associativity::Left, OperatorSemantics::min_op(), &builtin_infix_min};
    return std::nullopt;
}

constexpr std::optional<BuiltinFunctionSpec> find_builtin_function(std::string_view name) noexcept
{
    if (name == "sin") return BuiltinFunctionSpec{"sin", FunctionSemantics::pure(), &builtin_function_sin};
    if (name == "cos") return BuiltinFunctionSpec{"cos", FunctionSemantics::pure(), &builtin_function_cos};
    if (name == "tan") return BuiltinFunctionSpec{"tan", FunctionSemantics::pure(), &builtin_function_tan};
    if (name == "asin") return BuiltinFunctionSpec{"asin", FunctionSemantics::pure(), &builtin_function_asin};
    if (name == "acos") return BuiltinFunctionSpec{"acos", FunctionSemantics::pure(), &builtin_function_acos};
    if (name == "atan") return BuiltinFunctionSpec{"atan", FunctionSemantics::pure(), &builtin_function_atan};
    if (name == "sqrt") return BuiltinFunctionSpec{"sqrt", FunctionSemantics::pure(), &builtin_function_sqrt};
    if (name == "ceil") return BuiltinFunctionSpec{"ceil", FunctionSemantics::pure(), &builtin_function_ceil};
    if (name == "floor") return BuiltinFunctionSpec{"floor", FunctionSemantics::pure(), &builtin_function_floor};
    if (name == "round") return BuiltinFunctionSpec{"round", FunctionSemantics::pure(), &builtin_function_round};
    if (name == "exp") return BuiltinFunctionSpec{"exp", FunctionSemantics::pure(), &builtin_function_exp};
    if (name == "abs") return BuiltinFunctionSpec{"abs", FunctionSemantics::pure(), &builtin_function_abs};
    if (name == "rad") return BuiltinFunctionSpec{"rad", FunctionSemantics::pure(), &builtin_function_rad};
    if (name == "deg") return BuiltinFunctionSpec{"deg", FunctionSemantics::pure(), &builtin_function_deg};
    if (name == "max") return BuiltinFunctionSpec{"max", FunctionSemantics::pure(), &builtin_function_max};
    if (name == "min") return BuiltinFunctionSpec{"min", FunctionSemantics::pure(), &builtin_function_min};
    return std::nullopt;
}

constexpr BuiltinPrefixOperatorSpec require_builtin_prefix_spec(std::string_view symbol)
{
    if (const auto spec = find_builtin_prefix_operator(symbol))
    {
        return *spec;
    }
    throw std::logic_error("string_math: unknown builtin prefix operator");
}

constexpr BuiltinPostfixOperatorSpec require_builtin_postfix_spec(std::string_view symbol)
{
    if (const auto spec = find_builtin_postfix_operator(symbol))
    {
        return *spec;
    }
    throw std::logic_error("string_math: unknown builtin postfix operator");
}

constexpr BuiltinInfixOperatorSpec require_builtin_infix_spec(std::string_view symbol)
{
    if (const auto spec = find_builtin_infix_operator(symbol))
    {
        return *spec;
    }
    throw std::logic_error("string_math: unknown builtin infix operator");
}

constexpr BuiltinFunctionSpec require_builtin_function_spec(std::string_view name)
{
    if (const auto spec = find_builtin_function(name))
    {
        return *spec;
    }
    throw std::logic_error("string_math: unknown builtin function");
}

} // namespace string_math::detail
