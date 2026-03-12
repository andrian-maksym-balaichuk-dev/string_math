#pragma once

#include <type_traits>

#include <string_math/calculator.hpp>
#include <string_math/constexpr_context.hpp>
#include <string_math/detail/builtins.hpp>
#include <string_math/detail/constexpr.hpp>
#include <string_math/expression.hpp>

namespace string_math
{

inline const MathContext& default_context()
{
    static const MathContext context = MathContext::with_builtins();
    return context;
}

inline Result<MathValue> try_evaluate(std::string_view expression, const MathContext& context = default_context())
{
    const auto operation_result = string_math::try_create_operation(expression, context);
    if (!operation_result)
    {
        return operation_result.error();
    }
    return try_evaluate_operation(operation_result.value(), context);
}

inline Result<MathValue> try_evaluate(const Operation& operation, const MathContext& context)
{
    return try_evaluate_operation(operation, context);
}

inline MathValue evaluate(std::string_view expression, const MathContext& context = default_context())
{
    return try_evaluate(expression, context).value();
}

inline MathValue evaluate(const Operation& operation, const MathContext& context)
{
    return try_evaluate(operation, context).value();
}

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
template <std::size_t N>
constexpr MathValue evaluate(const char (&expression)[N])
{
    return detail::ConstexprParser(std::string_view(expression, N - 1)).parse();
}

template <class Context, class = void>
struct is_constexpr_context : std::false_type
{};

template <class Context>
struct is_constexpr_context<Context, std::void_t<decltype(Context::is_constexpr_context)>> : std::bool_constant<Context::is_constexpr_context>
{};

template <std::size_t N, class Context, std::enable_if_t<is_constexpr_context<Context>::value, int> = 0>
constexpr MathValue evaluate(const char (&expression)[N], const Context& context)
{
    return detail::ConstexprParser<Context>(std::string_view(expression, N - 1), &context).parse();
}

constexpr MathValue operator""_math(const char* expression, std::size_t size)
{
    return detail::ConstexprParser(std::string_view(expression, size)).parse();
}

template <detail::fixed_string Expression>
consteval MathValue evaluate_ct()
{
    return detail::ConstexprParser(Expression.view()).parse();
}
#else
inline MathValue operator""_math(const char* expression, std::size_t size)
{
    return evaluate(std::string_view(expression, size));
}
#endif

inline MathExpr to_math_expr(std::string expression, const MathContext& context = MathContext::with_builtins())
{
    return MathExpr(std::move(expression), context);
}

} // namespace string_math
