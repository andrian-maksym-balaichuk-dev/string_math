#pragma once

#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <string_math/context.hpp>

namespace string_math
{

namespace detail
{

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
    return value.visit([](const auto& current) -> MathValue {
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
    return value.visit([](const auto& current) -> MathValue {
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

constexpr auto require_builtin_prefix_invoker(std::string_view symbol) -> fw::static_function_ref<MathValue(const MathValue&)>
{
    if (symbol == "+") return fw::static_function_ref<MathValue(const MathValue&)>{ &builtin_prefix_identity };
    if (symbol == "-") return fw::static_function_ref<MathValue(const MathValue&)>{ &builtin_prefix_negate };
    if (symbol == "!") return fw::static_function_ref<MathValue(const MathValue&)>{ &builtin_prefix_logical_not };
    if (symbol == "abs") return fw::static_function_ref<MathValue(const MathValue&)>{ &builtin_prefix_abs };
    if (symbol == "sqrt") return fw::static_function_ref<MathValue(const MathValue&)>{ &builtin_prefix_sqrt };
    throw std::logic_error("string_math: unknown builtin prefix operator");
}

constexpr auto require_builtin_postfix_invoker(std::string_view symbol) -> fw::static_function_ref<MathValue(const MathValue&)>
{
    if (symbol == "!") return fw::static_function_ref<MathValue(const MathValue&)>{ &builtin_postfix_factorial };
    throw std::logic_error("string_math: unknown builtin postfix operator");
}

constexpr auto require_builtin_infix_invoker(std::string_view symbol)
    -> fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>
{
    if (symbol == "+") return fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>{ &builtin_infix_add };
    if (symbol == "-") return fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>{ &builtin_infix_subtract };
    if (symbol == "*") return fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>{ &builtin_infix_multiply };
    if (symbol == "/") return fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>{ &builtin_infix_divide };
    if (symbol == "%") return fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>{ &builtin_infix_modulo };
    if (symbol == "^") return fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>{ &builtin_infix_power };
    if (symbol == "log") return fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>{ &builtin_infix_log };
    if (symbol == "==") return fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>{ &builtin_infix_equal };
    if (symbol == "!=") return fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>{ &builtin_infix_not_equal };
    if (symbol == "<") return fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>{ &builtin_infix_less };
    if (symbol == "<=") return fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>{ &builtin_infix_less_equal };
    if (symbol == ">") return fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>{ &builtin_infix_greater };
    if (symbol == ">=") return fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>{ &builtin_infix_greater_equal };
    if (symbol == "&&") return fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>{ &builtin_infix_logical_and };
    if (symbol == "||") return fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>{ &builtin_infix_logical_or };
    if (symbol == "max") return fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>{ &builtin_infix_max };
    if (symbol == "min") return fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>{ &builtin_infix_min };
    throw std::logic_error("string_math: unknown builtin infix operator");
}

constexpr auto require_builtin_function_invoker(std::string_view name)
    -> fw::static_function_ref<MathValue(const MathValue*, std::size_t)>
{
    if (name == "sin") return fw::static_function_ref<MathValue(const MathValue*, std::size_t)>{ &builtin_function_sin };
    if (name == "cos") return fw::static_function_ref<MathValue(const MathValue*, std::size_t)>{ &builtin_function_cos };
    if (name == "tan") return fw::static_function_ref<MathValue(const MathValue*, std::size_t)>{ &builtin_function_tan };
    if (name == "asin") return fw::static_function_ref<MathValue(const MathValue*, std::size_t)>{ &builtin_function_asin };
    if (name == "acos") return fw::static_function_ref<MathValue(const MathValue*, std::size_t)>{ &builtin_function_acos };
    if (name == "atan") return fw::static_function_ref<MathValue(const MathValue*, std::size_t)>{ &builtin_function_atan };
    if (name == "sqrt") return fw::static_function_ref<MathValue(const MathValue*, std::size_t)>{ &builtin_function_sqrt };
    if (name == "ceil") return fw::static_function_ref<MathValue(const MathValue*, std::size_t)>{ &builtin_function_ceil };
    if (name == "floor") return fw::static_function_ref<MathValue(const MathValue*, std::size_t)>{ &builtin_function_floor };
    if (name == "round") return fw::static_function_ref<MathValue(const MathValue*, std::size_t)>{ &builtin_function_round };
    if (name == "exp") return fw::static_function_ref<MathValue(const MathValue*, std::size_t)>{ &builtin_function_exp };
    if (name == "abs") return fw::static_function_ref<MathValue(const MathValue*, std::size_t)>{ &builtin_function_abs };
    if (name == "rad") return fw::static_function_ref<MathValue(const MathValue*, std::size_t)>{ &builtin_function_rad };
    if (name == "deg") return fw::static_function_ref<MathValue(const MathValue*, std::size_t)>{ &builtin_function_deg };
    if (name == "max") return fw::static_function_ref<MathValue(const MathValue*, std::size_t)>{ &builtin_function_max };
    if (name == "min") return fw::static_function_ref<MathValue(const MathValue*, std::size_t)>{ &builtin_function_min };
    throw std::logic_error("string_math: unknown builtin function");
}

template <class... Ts, class F>
MathContext register_builtin_infix(
    MathContext context,
    std::string_view symbol,
    F&& function,
    int precedence,
    Associativity associativity = Associativity::Left,
    CallableSemantics semantics = {})
{
    context.add_infix_operator_for<Ts...>(
        symbol, std::forward<F>(function), precedence, associativity, semantics);
    return context;
}

template <class... Ts, class F>
MathContext register_builtin_prefix(
    MathContext context,
    std::string_view symbol,
    F&& function,
    int precedence = Precedence::Prefix,
    CallableSemantics semantics = {})
{
    context.add_prefix_operator_for<Ts...>(
        symbol, std::forward<F>(function), precedence, semantics);
    return context;
}

template <class... Ts, class F>
MathContext register_builtin_postfix(
    MathContext context,
    std::string_view symbol,
    F&& function,
    int precedence = Precedence::Postfix,
    CallableSemantics semantics = {})
{
    context.add_postfix_operator_for<Ts...>(
        symbol, std::forward<F>(function), precedence, semantics);
    return context;
}

template <class... Ts, class F>
MathContext register_builtin_function(
    MathContext context,
    std::string_view name,
    F&& function,
    CallableSemantics semantics = {})
{
    context.add_function_for<Ts...>(name, std::forward<F>(function), semantics);
    return context;
}

template <class Sig, class F>
MathContext register_builtin_function_signature(
    MathContext context,
    std::string_view name,
    F&& function,
    CallableSemantics semantics = {})
{
    context.template add_function<Sig>(name, std::forward<F>(function), semantics);
    return context;
}

template <class... Sigs, class F>
MathContext register_builtin_postfix_overloads(
    MathContext context,
    std::string_view symbol,
    F&& function,
    int precedence = Precedence::Postfix,
    CallableSemantics semantics = {})
{
    context.template add_postfix_operator_overloads<Sigs...>(
        symbol, std::forward<F>(function), precedence, semantics);
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
    context.add_variadic_function(name, min_arity, std::forward<F>(function), semantics);
    return context;
}

inline MathContext register_builtin_value(MathContext context, std::string_view name, MathValue value)
{
    context.set_value(name, value);
    return context;
}

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
template <
    class... Ts,
    std::size_t VariableCount,
    std::size_t InfixCount,
    std::size_t FunctionCount,
    std::size_t PrefixCount,
    std::size_t PostfixCount,
    class F>
constexpr auto register_builtin_infix(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view symbol,
    F&&,
    int precedence,
    Associativity associativity = Associativity::Left,
    CallableSemantics semantics = {})
{
    (void)sizeof...(Ts);
    return context.detail_with_infix_entry(
        symbol,
        precedence,
        associativity,
        require_builtin_infix_invoker(symbol),
        semantics);
}

template <
    class... Ts,
    std::size_t VariableCount,
    std::size_t InfixCount,
    std::size_t FunctionCount,
    std::size_t PrefixCount,
    std::size_t PostfixCount,
    class F>
constexpr auto register_builtin_prefix(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view symbol,
    F&&,
    int precedence = Precedence::Prefix,
    CallableSemantics semantics = {})
{
    (void)sizeof...(Ts);
    return context.detail_with_prefix_entry(
        symbol,
        precedence,
        require_builtin_prefix_invoker(symbol),
        semantics);
}

template <
    class... Ts,
    std::size_t VariableCount,
    std::size_t InfixCount,
    std::size_t FunctionCount,
    std::size_t PrefixCount,
    std::size_t PostfixCount,
    class F>
constexpr auto register_builtin_postfix(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view symbol,
    F&&,
    int precedence = Precedence::Postfix,
    CallableSemantics semantics = {})
{
    (void)sizeof...(Ts);
    return context.detail_with_postfix_entry(
        symbol,
        precedence,
        require_builtin_postfix_invoker(symbol),
        semantics);
}

template <
    class... Ts,
    std::size_t VariableCount,
    std::size_t InfixCount,
    std::size_t FunctionCount,
    std::size_t PrefixCount,
    std::size_t PostfixCount,
    class F>
constexpr auto register_builtin_function(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view name,
    F&&,
    CallableSemantics semantics = {})
{
    (void)sizeof...(Ts);
    return context.detail_with_function_entry(
        name,
        require_builtin_function_invoker(name),
        semantics);
}

template <
    class Sig,
    std::size_t VariableCount,
    std::size_t InfixCount,
    std::size_t FunctionCount,
    std::size_t PrefixCount,
    std::size_t PostfixCount,
    class F>
constexpr auto register_builtin_function_signature(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view name,
    F&&,
    CallableSemantics semantics = {})
{
    (void)sizeof(std::add_pointer_t<Sig>);
    return context.detail_with_function_entry(
        name,
        require_builtin_function_invoker(name),
        semantics);
}

template <
    class... Sigs,
    std::size_t VariableCount,
    std::size_t InfixCount,
    std::size_t FunctionCount,
    std::size_t PrefixCount,
    std::size_t PostfixCount,
    class F>
constexpr auto register_builtin_postfix_overloads(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view symbol,
    F&&,
    int precedence = Precedence::Postfix,
    CallableSemantics semantics = {})
{
    (void)sizeof...(Sigs);
    return context.detail_with_postfix_entry(
        symbol,
        precedence,
        require_builtin_postfix_invoker(symbol),
        semantics);
}

template <
    std::size_t VariableCount,
    std::size_t InfixCount,
    std::size_t FunctionCount,
    std::size_t PrefixCount,
    std::size_t PostfixCount,
    class F>
constexpr auto register_builtin_variadic_function(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view name,
    std::size_t,
    F&&,
    CallableSemantics semantics = {})
{
    return context.detail_with_function_entry(
        name,
        require_builtin_function_invoker(name),
        semantics);
}

template <
    std::size_t VariableCount,
    std::size_t InfixCount,
    std::size_t FunctionCount,
    std::size_t PrefixCount,
    std::size_t PostfixCount>
constexpr auto register_builtin_value(
    StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> context,
    std::string_view name,
    MathValue value)
{
    return context.detail_with_value_entry(name, value);
}
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
        std::move(context),
        "+",
        [](auto left, auto right) { return left + right; },
        Precedence::Additive,
        Associativity::Left,
        OperatorSemantics::arithmetic_add());
    const auto c02 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c01),
        "-",
        [](auto left, auto right) { return left - right; },
        Precedence::Additive,
        Associativity::Left,
        OperatorSemantics::arithmetic_subtract());
    const auto c03 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c02),
        "*",
        [](auto left, auto right) { return left * right; },
        Precedence::Multiplicative,
        Associativity::Left,
        OperatorSemantics::arithmetic_multiply());

    const auto c04 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c03),
        "/",
        [](auto left, auto right) {
            if (right == 0)
            {
                throw std::domain_error("string_math: division by zero");
            }
            return left / right;
        },
        Precedence::Multiplicative,
        Associativity::Left,
        OperatorSemantics::arithmetic_divide());

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
            {
                return left % right;
            }
            else
            {
                return std::fmod(left, right);
            }
        },
        Precedence::Multiplicative,
        Associativity::Left,
        OperatorSemantics::arithmetic_modulo());

    const auto c06 = detail::register_builtin_infix<double, long double>(
        std::move(c05),
        "^",
        [](auto left, auto right) { return std::pow(left, right); },
        Precedence::Power,
        Associativity::Right,
        OperatorSemantics::power());
    const auto c07 = detail::register_builtin_infix<double, long double>(
        std::move(c06),
        "log",
        [](auto base, auto value) { return std::log(value) / std::log(base); },
        Precedence::Power,
        Associativity::Left,
        OperatorSemantics::logarithm());
    const auto c08 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c07),
        "==",
        [](auto left, auto right) { return left == right ? 1 : 0; },
        Precedence::Comparison,
        Associativity::Left,
        OperatorSemantics::comparison(ArithmeticKind::CompareEqual));
    const auto c09 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c08),
        "!=",
        [](auto left, auto right) { return left != right ? 1 : 0; },
        Precedence::Comparison,
        Associativity::Left,
        OperatorSemantics::comparison(ArithmeticKind::CompareNotEqual));
    const auto c10 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c09),
        "<",
        [](auto left, auto right) { return left < right ? 1 : 0; },
        Precedence::Comparison,
        Associativity::Left,
        OperatorSemantics::comparison(ArithmeticKind::CompareLess));
    const auto c11 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c10),
        "<=",
        [](auto left, auto right) { return left <= right ? 1 : 0; },
        Precedence::Comparison,
        Associativity::Left,
        OperatorSemantics::comparison(ArithmeticKind::CompareLessEqual));
    const auto c12 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c11),
        ">",
        [](auto left, auto right) { return left > right ? 1 : 0; },
        Precedence::Comparison,
        Associativity::Left,
        OperatorSemantics::comparison(ArithmeticKind::CompareGreater));
    const auto c13 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c12),
        ">=",
        [](auto left, auto right) { return left >= right ? 1 : 0; },
        Precedence::Comparison,
        Associativity::Left,
        OperatorSemantics::comparison(ArithmeticKind::CompareGreaterEqual));
    const auto c14 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c13),
        "&&",
        [](auto left, auto right) { return (left != 0 && right != 0) ? 1 : 0; },
        Precedence::LogicalAnd,
        Associativity::Left,
        OperatorSemantics::logical_and_op());
    const auto c15 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c14),
        "||",
        [](auto left, auto right) { return (left != 0 || right != 0) ? 1 : 0; },
        Precedence::LogicalOr,
        Associativity::Left,
        OperatorSemantics::logical_or_op());
    const auto c16 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c15),
        "max",
        [](auto left, auto right) { return std::max(left, right); },
        Precedence::Power,
        Associativity::Left,
        OperatorSemantics::max_op());
    const auto c17 = detail::register_builtin_infix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c16),
        "min",
        [](auto left, auto right) { return std::min(left, right); },
        Precedence::Power,
        Associativity::Left,
        OperatorSemantics::min_op());

    const auto c18 = detail::register_builtin_prefix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c17), "+", [](auto value) { return value; }, Precedence::Prefix, OperatorSemantics::identity());
    const auto c19 = detail::register_builtin_prefix<STRING_MATH_SIGNED_AND_FLOATING_VALUE_TYPES>(
        std::move(c18), "-", [](auto value) { return -value; }, Precedence::Prefix, OperatorSemantics::negate());
    const auto c20 = detail::register_builtin_prefix<STRING_MATH_ALL_VALUE_TYPES>(
        std::move(c19), "!", [](auto value) { return value == 0 ? 1 : 0; }, Precedence::Prefix, OperatorSemantics::logical_not_op());
    const auto c21 = detail::register_builtin_postfix_overloads<STRING_MATH_SIGNED_FACTORIAL_SIGNATURES, STRING_MATH_UNSIGNED_FACTORIAL_SIGNATURES>(
        std::move(c20),
        "!",
        [](auto value) {
            using value_t = std::decay_t<decltype(value)>;
            return static_cast<value_t>(detail::factorial_value(static_cast<long double>(value)));
        },
        Precedence::Postfix,
        OperatorSemantics::factorial());
    const auto c22 = detail::register_builtin_postfix<double, long double>(
        std::move(c21),
        "!",
        [](auto value) { return detail::factorial_value(value); },
        Precedence::Postfix,
        OperatorSemantics::factorial());

    const auto c23 = detail::register_builtin_function<float, double, long double>(
        std::move(c22), "sin", [](auto value) { return std::sin(value); }, FunctionSemantics::pure());
    const auto c24 = detail::register_builtin_function<float, double, long double>(
        std::move(c23), "cos", [](auto value) { return std::cos(value); }, FunctionSemantics::pure());
    const auto c25 = detail::register_builtin_function<float, double, long double>(
        std::move(c24), "tan", [](auto value) { return std::tan(value); }, FunctionSemantics::pure());
    const auto c26 = detail::register_builtin_function<float, double, long double>(
        std::move(c25), "asin", [](auto value) { return std::asin(value); }, FunctionSemantics::pure());
    const auto c27 = detail::register_builtin_function<float, double, long double>(
        std::move(c26), "acos", [](auto value) { return std::acos(value); }, FunctionSemantics::pure());
    const auto c28 = detail::register_builtin_function<float, double, long double>(
        std::move(c27), "atan", [](auto value) { return std::atan(value); }, FunctionSemantics::pure());
    const auto c29 = detail::register_builtin_function<float, double, long double>(
        std::move(c28), "sqrt", [](auto value) { return std::sqrt(value); }, FunctionSemantics::pure());
    const auto c30 = detail::register_builtin_function<float, double, long double>(
        std::move(c29), "ceil", [](auto value) { return std::ceil(value); }, FunctionSemantics::pure());
    const auto c31 = detail::register_builtin_function<float, double, long double>(
        std::move(c30), "floor", [](auto value) { return std::floor(value); }, FunctionSemantics::pure());
    const auto c32 = detail::register_builtin_function<float, double, long double>(
        std::move(c31), "round", [](auto value) { return std::round(value); }, FunctionSemantics::pure());
    const auto c33 = detail::register_builtin_function<float, double, long double>(
        std::move(c32), "exp", [](auto value) { return std::exp(value); }, FunctionSemantics::pure());
    const auto c34 = detail::register_builtin_function<STRING_MATH_SIGNED_AND_FLOATING_VALUE_TYPES>(
        std::move(c33), "abs", [](auto value) { return std::abs(value); }, FunctionSemantics::pure());
    const auto c35 = detail::register_builtin_function_signature<double(double)>(
        std::move(c34),
        "rad",
        [](double value) { return value * (3.14159265358979323846 / 180.0); },
        FunctionSemantics::pure());
    const auto c36 = detail::register_builtin_function_signature<double(double)>(
        std::move(c35),
        "deg",
        [](double value) { return value * (180.0 / 3.14159265358979323846); },
        FunctionSemantics::pure());
    const auto c37 = detail::register_builtin_variadic_function(
        std::move(c36),
        "max",
        1,
        [](const std::vector<MathValue>& arguments) {
            MathValue current = arguments.front();
            for (std::size_t index = 1; index < arguments.size(); ++index)
            {
                if (arguments[index] > current)
                {
                    current = arguments[index];
                }
            }
            return current;
        },
        FunctionSemantics::pure());
    const auto c38 = detail::register_builtin_variadic_function(
        std::move(c37),
        "min",
        1,
        [](const std::vector<MathValue>& arguments) {
            MathValue current = arguments.front();
            for (std::size_t index = 1; index < arguments.size(); ++index)
            {
                if (arguments[index] < current)
                {
                    current = arguments[index];
                }
            }
            return current;
        },
        FunctionSemantics::pure());
    const auto c39 = detail::register_builtin_prefix<STRING_MATH_SIGNED_AND_FLOATING_VALUE_TYPES>(
        std::move(c38), "abs", [](auto value) { return std::abs(value); }, Precedence::Prefix, FunctionSemantics::pure());
    const auto c40 = detail::register_builtin_prefix<float, double, long double>(
        std::move(c39), "sqrt", [](auto value) { return std::sqrt(value); }, Precedence::Prefix, FunctionSemantics::pure());
    return add_builtin_constants(std::move(c40));
}

inline MathContext MathContext::with_builtins()
{
    return string_math::with_builtins(MathContext{});
}

} // namespace string_math
