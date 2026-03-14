#pragma once

#include <string_math/expression/operation.hpp>

namespace string_math
{

class Calculator
{
public:
    Calculator() : m_context(MathContext::with_builtins()) {}
    explicit Calculator(MathContext context) : m_context(std::move(context)) {}

    Calculator& set_context(MathContext context)
    {
        m_context = std::move(context);
        return *this;
    }

    Calculator& set_value(std::string_view name, MathValue value)
    {
        m_context.set_value(name, value);
        return *this;
    }

    Calculator& add_variable(std::string_view name, MathValue value)
    {
        m_context.add_variable(name, value);
        return *this;
    }

    Calculator& register_variable(std::string_view name, MathValue value)
    {
        return add_variable(name, value);
    }

    bool remove_variable(std::string_view name)
    {
        return m_context.remove_variable(name);
    }

    MathValue evaluate(std::string_view expression) const
    {
        return try_evaluate(expression).value();
    }

    MathValue evaluate(const Operation& operation) const
    {
        return try_evaluate(operation).value();
    }

    Result<MathValue> try_evaluate(std::string_view expression) const
    {
        const auto operation_result = string_math::try_create_operation(expression, m_context);
        if (!operation_result)
        {
            return operation_result.error();
        }
        return try_evaluate_operation(operation_result.value(), m_context);
    }

    Result<MathValue> try_evaluate(const Operation& operation) const
    {
        return try_evaluate_operation(operation, m_context);
    }

    template <class T>
    T evaluate_as(std::string_view expression) const
    {
        return cast_value<T>(evaluate(expression), m_context.policy().narrowing);
    }

    template <class T>
    T evaluate_as(const Operation& operation) const
    {
        return cast_value<T>(evaluate(operation), m_context.policy().narrowing);
    }

    Operation create_operation(std::string_view expression) const
    {
        return try_create_operation(expression).value();
    }

    Result<Operation> try_create_operation(std::string_view expression) const
    {
        return string_math::try_create_operation(expression, m_context);
    }

    MathValue operator[](const std::string& name) const
    {
        const auto value = m_context.find_value(name);
        if (!value)
        {
            throw std::invalid_argument("string_math: unknown variable '" + name + "'");
        }
        return *value;
    }

    MathContext& mutable_context() noexcept
    {
        return m_context;
    }

    MathContext& context() noexcept
    {
        return m_context;
    }

    const MathContext& context() const noexcept
    {
        return m_context;
    }

    FrozenMathContext snapshot() const
    {
        return m_context.snapshot();
    }

private:
    MathContext m_context;
};

} // namespace string_math
