#pragma once

#include <string_math/calculator.hpp>

namespace string_math
{

class SMath
{
public:
    static Calculator& instance()
    {
        static Calculator calculator;
        return calculator;
    }

    static Calculator& calculator()
    {
        return instance();
    }

    static MathContext& context()
    {
        return instance().context();
    }

    static void set_context(MathContext context)
    {
        instance().set_context(std::move(context));
    }

    static MathValue evaluate(std::string_view expression)
    {
        return instance().evaluate(expression);
    }

    static Result<MathValue> try_evaluate(std::string_view expression)
    {
        return instance().try_evaluate(expression);
    }

    static MathValue evaluate(const Operation& operation)
    {
        return instance().evaluate(operation);
    }

    static Result<MathValue> try_evaluate(const Operation& operation)
    {
        return instance().try_evaluate(operation);
    }

    template <class T>
    static T evaluate_as(std::string_view expression)
    {
        return instance().template evaluate_as<T>(expression);
    }

    static Operation create_operation(std::string_view expression)
    {
        return instance().create_operation(expression);
    }

    static Result<Operation> try_create_operation(std::string_view expression)
    {
        return instance().try_create_operation(expression);
    }

    static void set_value(std::string_view name, MathValue value)
    {
        instance().context().set_value(name, value);
    }
};

} // namespace string_math
