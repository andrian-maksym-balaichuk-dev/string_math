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

    static void set_value(std::string name, MathValue value)
    {
        instance().set_value(std::move(name), value);
    }

    template <class Sig, class F>
    static void add_function(std::string name, F&& function)
    {
        instance().template add_function<Sig>(std::move(name), std::forward<F>(function));
    }

    template <class F>
    static void add_function(std::string name, F&& function)
    {
        instance().add_function(std::move(name), std::forward<F>(function));
    }

    template <class... Sigs, class F>
    static void add_function_overloads(std::string name, F&& function)
    {
        instance().template add_function_overloads<Sigs...>(std::move(name), std::forward<F>(function));
    }

    template <class... Ts, class F>
    static void add_function_for(std::string name, F&& function)
    {
        instance().template add_function_for<Ts...>(std::move(name), std::forward<F>(function));
    }

    template <class... Ts, class F>
    static void add_binary_function_for(std::string name, F&& function)
    {
        instance().template add_binary_function_for<Ts...>(std::move(name), std::forward<F>(function));
    }

    template <class Sig, class F>
    static void add_prefix_operator(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        instance().template add_prefix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class F>
    static void add_prefix_operator(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        instance().add_prefix_operator(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class... Sigs, class F>
    static void add_prefix_operator_overloads(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        instance().template add_prefix_operator_overloads<Sigs...>(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class... Ts, class F>
    static void add_prefix_operator_for(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        instance().template add_prefix_operator_for<Ts...>(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class Sig, class F>
    static void add_postfix_operator(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        instance().template add_postfix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class F>
    static void add_postfix_operator(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        instance().add_postfix_operator(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class... Sigs, class F>
    static void add_postfix_operator_overloads(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        instance().template add_postfix_operator_overloads<Sigs...>(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class... Ts, class F>
    static void add_postfix_operator_for(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        instance().template add_postfix_operator_for<Ts...>(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class Sig, class F>
    static void add_infix_operator(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        instance().template add_infix_operator<Sig>(
            std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
    }

    template <class F>
    static void add_infix_operator(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        instance().add_infix_operator(std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
    }

    template <class... Sigs, class F>
    static void add_infix_operator_overloads(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        instance().template add_infix_operator_overloads<Sigs...>(
            std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
    }

    template <class... Ts, class F>
    static void add_infix_operator_for(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        instance().template add_infix_operator_for<Ts...>(
            std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
    }

    template <class F>
    static void add_literal_parser(std::string prefix, std::string suffix, F&& parser)
    {
        instance().add_literal_parser(std::move(prefix), std::move(suffix), std::forward<F>(parser));
    }
};

} // namespace string_math
