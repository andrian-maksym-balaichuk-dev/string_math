#pragma once

#include <string_math/expression/operation.hpp>

namespace string_math
{

class MathExpr
{
public:
    explicit MathExpr(std::string expression, MathContext context = MathContext::with_builtins())
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

} // namespace string_math
