#pragma once

#include <type_traits>

#include <string_math/calculator.hpp>
#include <string_math/internal/context/static_context.hpp>
#include <string_math/internal/parser/parser.hpp>
#include <string_math/internal/expression/operation.hpp>

namespace string_math
{

class MathExpr
{
public:
    explicit MathExpr(std::string expression, MathContext context)
    : m_expression(std::move(expression)), m_parent_context(std::move(context)),
      m_local_context(m_parent_context, true), m_operation(string_math::try_parse_operation(m_expression, m_local_context).value().operation)
    {}

    MathExpr& substitute(std::string name, MathValue value)
    {
        m_local_context.set_value(std::move(name), value);
        return *this;
    }

    Result<MathValue> try_eval() const
    {
        return try_evaluate_operation(m_operation, m_local_context);
    }

    Result<MathValue> try_result() const
    {
        return try_eval();
    }

    MathValue eval() const
    {
        return try_eval().value();
    }

    MathValue result() const
    {
        return eval();
    }

    template <class T>
    T result_as() const
    {
        return cast_value<T>(eval(), m_local_context.policy().narrowing);
    }

    std::vector<std::string> variables() const
    {
        return m_operation.variables();
    }

    std::vector<std::string> local_variables() const
    {
        std::vector<std::string> names;
        for (const auto& name : m_operation.variables())
        {
            const auto value = m_local_context.find_value(name);
            const auto parent_value = m_parent_context.find_value(name);
            if (value && (!parent_value || *value != *parent_value))
            {
                names.push_back(name);
            }
        }
        return names;
    }

    const MathContext& context() const noexcept
    {
        return m_local_context;
    }

    MathContext& mutable_context() noexcept
    {
        return m_local_context;
    }

    template <class... Ts, class... Names>
    auto compile(Names&&... names) const -> fw::function_wrapper<MathValue(Ts...)>
    {
        static_assert(sizeof...(Ts) == sizeof...(Names), "compile names/types mismatch");
        const auto operation = m_operation;
        const auto context = m_local_context;
        const std::array<std::string, sizeof...(Ts)> variable_names{ std::string(std::forward<Names>(names))... };
        return fw::function_wrapper<MathValue(Ts...)>([operation, context, variable_names](Ts... values) -> MathValue {
            MathContext call_context(context);
            std::size_t index = 0;
            (call_context.set_value(variable_names[index++], MathValue(values)), ...);
            return evaluate_operation(operation, call_context);
        });
    }

private:
    std::string m_expression;
    MathContext m_parent_context;
    MathContext m_local_context;
    Operation m_operation;
};

inline Result<MathValue> try_evaluate(std::string_view expression, const MathContext& context)
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

inline MathValue evaluate(std::string_view expression, const MathContext& context)
{
    return try_evaluate(expression, context).value();
}

inline MathValue evaluate(const Operation& operation, const MathContext& context)
{
    return try_evaluate(operation, context).value();
}

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
template <class Context, class = void>
struct is_constexpr_context : std::false_type
{};

template <class Context>
struct is_constexpr_context<Context, std::void_t<decltype(Context::is_constexpr_context)>> : std::bool_constant<Context::is_constexpr_context>
{};

template <std::size_t N, class Context, std::enable_if_t<is_constexpr_context<Context>::value, int> = 0>
constexpr MathValue evaluate(const char (&expression)[N], const Context& context)
{
    return internal::ConstexprParser<Context>(std::string_view(expression, N - 1), &context).parse();
}
#endif

inline MathExpr to_math_expr(std::string expression, const MathContext& context)
{
    return MathExpr(std::move(expression), context);
}

} // namespace string_math
