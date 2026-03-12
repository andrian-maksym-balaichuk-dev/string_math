#pragma once

#include <string_math/operation.hpp>

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

    MathExpr& set_value(std::string name, MathValue value)
    {
        m_local_context.set_value(std::move(name), value);
        return *this;
    }

    MathExpr& register_variable(std::string name, MathValue value)
    {
        return set_value(std::move(name), value);
    }

    template <class Sig, class F>
    MathExpr& add_function(std::string name, F&& function)
    {
        m_local_context.template add_function<Sig>(std::move(name), std::forward<F>(function));
        return *this;
    }

    template <class F>
    MathExpr& add_function(std::string name, F&& function)
    {
        m_local_context.add_function(std::move(name), std::forward<F>(function));
        return *this;
    }

    template <class Sig, class F>
    MathExpr& register_function(std::string name, F&& function)
    {
        return add_function<Sig>(std::move(name), std::forward<F>(function));
    }

    template <class F>
    MathExpr& register_function(std::string name, F&& function)
    {
        return add_function(std::move(name), std::forward<F>(function));
    }

    template <class... Sigs, class F>
    MathExpr& add_function_overloads(std::string name, F&& function)
    {
        m_local_context.template add_function_overloads<Sigs...>(std::move(name), std::forward<F>(function));
        return *this;
    }

    template <class... Ts, class F>
    MathExpr& add_function_for(std::string name, F&& function)
    {
        m_local_context.template add_function_for<Ts...>(std::move(name), std::forward<F>(function));
        return *this;
    }

    template <class... Ts, class F>
    MathExpr& add_binary_function_for(std::string name, F&& function)
    {
        m_local_context.template add_binary_function_for<Ts...>(std::move(name), std::forward<F>(function));
        return *this;
    }

    template <class F>
    MathExpr& add_variadic_function(std::string name, std::size_t min_arity, F&& function)
    {
        m_local_context.add_variadic_function(std::move(name), min_arity, std::forward<F>(function));
        return *this;
    }

    template <class T, class F>
    MathExpr& add_variadic_function_for(std::string name, std::size_t min_arity, F&& function)
    {
        m_local_context.template add_variadic_function_for<T>(std::move(name), min_arity, std::forward<F>(function));
        return *this;
    }

    template <class F>
    MathExpr& register_variadic_function(std::string name, std::size_t min_arity, F&& function)
    {
        return add_variadic_function(std::move(name), min_arity, std::forward<F>(function));
    }

    template <class T, class F>
    MathExpr& register_variadic_function_for(std::string name, std::size_t min_arity, F&& function)
    {
        return add_variadic_function_for<T>(std::move(name), min_arity, std::forward<F>(function));
    }

    template <class Sig, class F>
    MathExpr& add_prefix_operator(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        m_local_context.template add_prefix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence);
        return *this;
    }

    template <class F>
    MathExpr& add_prefix_operator(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        m_local_context.add_prefix_operator(std::move(symbol), std::forward<F>(function), precedence);
        return *this;
    }

    template <class Sig, class F>
    MathExpr& register_prefix_operator(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        return add_prefix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class F>
    MathExpr& register_prefix_operator(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        return add_prefix_operator(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class... Sigs, class F>
    MathExpr& add_prefix_operator_overloads(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        m_local_context.template add_prefix_operator_overloads<Sigs...>(std::move(symbol), std::forward<F>(function), precedence);
        return *this;
    }

    template <class... Ts, class F>
    MathExpr& add_prefix_operator_for(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        m_local_context.template add_prefix_operator_for<Ts...>(std::move(symbol), std::forward<F>(function), precedence);
        return *this;
    }

    template <class Sig, class F>
    MathExpr& add_postfix_operator(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        m_local_context.template add_postfix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence);
        return *this;
    }

    template <class F>
    MathExpr& add_postfix_operator(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        m_local_context.add_postfix_operator(std::move(symbol), std::forward<F>(function), precedence);
        return *this;
    }

    template <class Sig, class F>
    MathExpr& register_postfix_operator(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        return add_postfix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class F>
    MathExpr& register_postfix_operator(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        return add_postfix_operator(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class... Sigs, class F>
    MathExpr& add_postfix_operator_overloads(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        m_local_context.template add_postfix_operator_overloads<Sigs...>(std::move(symbol), std::forward<F>(function), precedence);
        return *this;
    }

    template <class... Ts, class F>
    MathExpr& add_postfix_operator_for(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        m_local_context.template add_postfix_operator_for<Ts...>(std::move(symbol), std::forward<F>(function), precedence);
        return *this;
    }

    template <class Sig, class F>
    MathExpr& add_infix_operator(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        m_local_context.template add_infix_operator<Sig>(
            std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
        return *this;
    }

    template <class F>
    MathExpr& add_infix_operator(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        m_local_context.add_infix_operator(
            std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
        return *this;
    }

    template <class Sig, class F>
    MathExpr& register_infix_operator(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        return add_infix_operator<Sig>(
            std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
    }

    template <class F>
    MathExpr& register_infix_operator(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        return add_infix_operator(std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
    }

    template <class... Sigs, class F>
    MathExpr&
    add_infix_operator_overloads(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        m_local_context.template add_infix_operator_overloads<Sigs...>(
            std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
        return *this;
    }

    template <class... Ts, class F>
    MathExpr& add_infix_operator_for(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        m_local_context.template add_infix_operator_for<Ts...>(
            std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
        return *this;
    }

    template <class Sig, class F, class PolicyF>
    MathExpr& add_infix_operator_with_policy(
        std::string symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        m_local_context.template add_infix_operator_with_policy<Sig>(
            std::move(symbol),
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
        return *this;
    }

    template <class F, class PolicyF>
    MathExpr& add_infix_operator_with_policy(
        std::string symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        m_local_context.add_infix_operator_with_policy(
            std::move(symbol),
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
        return *this;
    }

    template <class Sig, class F, class PolicyF>
    MathExpr& register_infix_operator_with_policy(
        std::string symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator_with_policy<Sig>(
            std::move(symbol),
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
    }

    template <class F, class PolicyF>
    MathExpr& register_infix_operator_with_policy(
        std::string symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator_with_policy(
            std::move(symbol),
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
    }

    template <class F>
    MathExpr& add_literal_parser(std::string prefix, std::string suffix, F&& parser)
    {
        m_local_context.add_literal_parser(std::move(prefix), std::move(suffix), std::forward<F>(parser));
        return *this;
    }

    template <class F>
    MathExpr& register_literal_parser(std::string prefix, std::string suffix, F&& parser)
    {
        return add_literal_parser(std::move(prefix), std::move(suffix), std::forward<F>(parser));
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
