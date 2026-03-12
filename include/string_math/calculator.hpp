#pragma once

#include <string_math/operation.hpp>

namespace string_math
{

class Calculator
{
public:
    Calculator() : m_context(MathContext::with_builtins()) {}
    explicit Calculator(MathContext context) : m_context(std::move(context)) {}

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

    Calculator& set_value(std::string name, MathValue value)
    {
        m_context.set_value(std::move(name), value);
        return *this;
    }
#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <std::size_t NameSize, class T>
    Calculator& set_value(const char (&name)[NameSize], T value)
    {
        m_context.set_value(name, value);
        return *this;
    }
#endif

    Calculator& add_variable(std::string name, MathValue value)
    {
        m_context.add_variable(std::move(name), value);
        return *this;
    }
#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <std::size_t NameSize, class T>
    Calculator& add_variable(const char (&name)[NameSize], T value)
    {
        m_context.add_variable(name, value);
        return *this;
    }
#endif

    Calculator& register_variable(std::string name, MathValue value)
    {
        return add_variable(std::move(name), value);
    }
#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <std::size_t NameSize, class T>
    Calculator& register_variable(const char (&name)[NameSize], T value)
    {
        return add_variable(name, value);
    }
#endif

    bool remove_variable(const std::string& name)
    {
        return m_context.remove_variable(name);
    }

    template <class Sig, class F>
    Calculator& add_function(std::string name, F&& function)
    {
        m_context.template add_function<Sig>(std::move(name), std::forward<F>(function));
        return *this;
    }

    template <class F>
    Calculator& add_function(std::string name, F&& function)
    {
        m_context.add_function(std::move(name), std::forward<F>(function));
        return *this;
    }
#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function, std::size_t NameSize>
    Calculator& add_function(const char (&name)[NameSize], CallableSemantics semantics = {})
    {
        m_context.template add_function<Sig, Function>(name, semantics);
        return *this;
    }

    template <auto Function, std::size_t NameSize>
    Calculator& add_function(const char (&name)[NameSize], CallableSemantics semantics = {})
    {
        m_context.template add_function<Function>(name, semantics);
        return *this;
    }

#endif

    template <class Sig, class F>
    Calculator& register_function(std::string name, F&& function)
    {
        return add_function<Sig>(std::move(name), std::forward<F>(function));
    }

    template <class F>
    Calculator& register_function(std::string name, F&& function)
    {
        return add_function(std::move(name), std::forward<F>(function));
    }
#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function, std::size_t NameSize>
    Calculator& register_function(const char (&name)[NameSize], CallableSemantics semantics = {})
    {
        return add_function<Sig, Function>(name, semantics);
    }

    template <auto Function, std::size_t NameSize>
    Calculator& register_function(const char (&name)[NameSize], CallableSemantics semantics = {})
    {
        return add_function<Function>(name, semantics);
    }

#endif

    template <class... Sigs, class F>
    Calculator& add_function_overloads(std::string name, F&& function)
    {
        m_context.template add_function_overloads<Sigs...>(std::move(name), std::forward<F>(function));
        return *this;
    }

    template <class... Ts, class F>
    Calculator& add_function_for(std::string name, F&& function)
    {
        m_context.template add_function_for<Ts...>(std::move(name), std::forward<F>(function));
        return *this;
    }

    template <class... Ts, class F>
    Calculator& add_binary_function_for(std::string name, F&& function)
    {
        m_context.template add_binary_function_for<Ts...>(std::move(name), std::forward<F>(function));
        return *this;
    }

    template <class F>
    Calculator& add_variadic_function(std::string name, std::size_t min_arity, F&& function)
    {
        m_context.add_variadic_function(std::move(name), min_arity, std::forward<F>(function));
        return *this;
    }

    template <class T, class F>
    Calculator& add_variadic_function_for(std::string name, std::size_t min_arity, F&& function)
    {
        m_context.template add_variadic_function_for<T>(std::move(name), min_arity, std::forward<F>(function));
        return *this;
    }

    template <class F>
    Calculator& register_variadic_function(std::string name, std::size_t min_arity, F&& function)
    {
        return add_variadic_function(std::move(name), min_arity, std::forward<F>(function));
    }

    template <class T, class F>
    Calculator& register_variadic_function_for(std::string name, std::size_t min_arity, F&& function)
    {
        return add_variadic_function_for<T>(std::move(name), min_arity, std::forward<F>(function));
    }

    bool remove_function(const std::string& name)
    {
        return m_context.remove_function(name);
    }

    template <class Sig, class F>
    Calculator& add_prefix_operator(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        m_context.template add_prefix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence);
        return *this;
    }

    template <class F>
    Calculator& add_prefix_operator(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        m_context.add_prefix_operator(std::move(symbol), std::forward<F>(function), precedence);
        return *this;
    }
#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function, std::size_t SymbolSize>
    Calculator& add_prefix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        m_context.template add_prefix_operator<Sig, Function>(symbol, precedence, semantics);
        return *this;
    }

    template <auto Function, std::size_t SymbolSize>
    Calculator& add_prefix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        m_context.template add_prefix_operator<Function>(symbol, precedence, semantics);
        return *this;
    }

#endif

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function, std::size_t SymbolSize>
    Calculator& register_prefix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        return add_prefix_operator<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function, std::size_t SymbolSize>
    Calculator& register_prefix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        return add_prefix_operator<Function>(symbol, precedence, semantics);
    }

#endif

    template <class Sig, class F>
    Calculator& register_prefix_operator(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        return add_prefix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class F>
    Calculator& register_prefix_operator(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        return add_prefix_operator(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class... Sigs, class F>
    Calculator& add_prefix_operator_overloads(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        m_context.template add_prefix_operator_overloads<Sigs...>(std::move(symbol), std::forward<F>(function), precedence);
        return *this;
    }

    template <class... Ts, class F>
    Calculator& add_prefix_operator_for(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        m_context.template add_prefix_operator_for<Ts...>(std::move(symbol), std::forward<F>(function), precedence);
        return *this;
    }

    template <class Sig, class F>
    Calculator& add_postfix_operator(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        m_context.template add_postfix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence);
        return *this;
    }

    template <class F>
    Calculator& add_postfix_operator(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        m_context.add_postfix_operator(std::move(symbol), std::forward<F>(function), precedence);
        return *this;
    }
#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function, std::size_t SymbolSize>
    Calculator& add_postfix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        m_context.template add_postfix_operator<Sig, Function>(symbol, precedence, semantics);
        return *this;
    }

    template <auto Function, std::size_t SymbolSize>
    Calculator& add_postfix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        m_context.template add_postfix_operator<Function>(symbol, precedence, semantics);
        return *this;
    }

#endif

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function, std::size_t SymbolSize>
    Calculator& register_postfix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        return add_postfix_operator<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function, std::size_t SymbolSize>
    Calculator& register_postfix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        return add_postfix_operator<Function>(symbol, precedence, semantics);
    }

#endif

    template <class Sig, class F>
    Calculator& register_postfix_operator(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        return add_postfix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class F>
    Calculator& register_postfix_operator(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        return add_postfix_operator(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class... Sigs, class F>
    Calculator& add_postfix_operator_overloads(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        m_context.template add_postfix_operator_overloads<Sigs...>(std::move(symbol), std::forward<F>(function), precedence);
        return *this;
    }

    template <class... Ts, class F>
    Calculator& add_postfix_operator_for(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        m_context.template add_postfix_operator_for<Ts...>(std::move(symbol), std::forward<F>(function), precedence);
        return *this;
    }

    template <class Sig, class F>
    Calculator&
    add_infix_operator(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        m_context.template add_infix_operator<Sig>(
            std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
        return *this;
    }

    template <class F>
    Calculator&
    add_infix_operator(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        m_context.add_infix_operator(std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
        return *this;
    }
#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function, std::size_t SymbolSize>
    Calculator& add_infix_operator(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        m_context.template add_infix_operator<Sig, Function>(symbol, precedence, associativity, semantics, binary_policy);
        return *this;
    }

    template <auto Function, std::size_t SymbolSize>
    Calculator& add_infix_operator(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        m_context.template add_infix_operator<Function>(symbol, precedence, associativity, semantics, binary_policy);
        return *this;
    }

#endif

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function, std::size_t SymbolSize>
    Calculator& register_infix_operator(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        return add_infix_operator<Sig, Function>(symbol, precedence, associativity, semantics, binary_policy);
    }

    template <auto Function, std::size_t SymbolSize>
    Calculator& register_infix_operator(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        return add_infix_operator<Function>(symbol, precedence, associativity, semantics, binary_policy);
    }

#endif

    template <class Sig, class F>
    Calculator&
    register_infix_operator(
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
    Calculator&
    register_infix_operator(
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
    Calculator&
    add_infix_operator_overloads(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        m_context.template add_infix_operator_overloads<Sigs...>(
            std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
        return *this;
    }

    template <class... Ts, class F>
    Calculator&
    add_infix_operator_for(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        m_context.template add_infix_operator_for<Ts...>(
            std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
        return *this;
    }

    template <class Sig, class F, class PolicyF>
    Calculator& add_infix_operator_with_policy(
        std::string symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        m_context.template add_infix_operator_with_policy<Sig>(
            std::move(symbol),
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
        return *this;
    }

    template <class F, class PolicyF>
    Calculator& add_infix_operator_with_policy(
        std::string symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        m_context.add_infix_operator_with_policy(
            std::move(symbol),
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
        return *this;
    }
#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <auto Function, auto PolicyHandler, std::size_t SymbolSize>
    Calculator& add_infix_operator_with_policy(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        m_context.template add_infix_operator_with_policy<Function, PolicyHandler>(symbol, precedence, associativity, semantics);
        return *this;
    }
#endif

    template <class Sig, class F, class PolicyF>
    Calculator& register_infix_operator_with_policy(
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
    Calculator& register_infix_operator_with_policy(
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
#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <auto Function, auto PolicyHandler, std::size_t SymbolSize>
    Calculator& register_infix_operator_with_policy(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator_with_policy<Function, PolicyHandler>(symbol, precedence, associativity, semantics);
    }
#endif

    bool remove_prefix_operator(const std::string& symbol)
    {
        return m_context.remove_prefix_operator(symbol);
    }

    bool remove_postfix_operator(const std::string& symbol)
    {
        return m_context.remove_postfix_operator(symbol);
    }

    bool remove_infix_operator(const std::string& symbol)
    {
        return m_context.remove_infix_operator(symbol);
    }

    template <class F>
    Calculator& add_literal_parser(std::string prefix, std::string suffix, F&& parser)
    {
        m_context.add_literal_parser(std::move(prefix), std::move(suffix), std::forward<F>(parser));
        return *this;
    }

    template <class F>
    Calculator& register_literal_parser(std::string prefix, std::string suffix, F&& parser)
    {
        return add_literal_parser(std::move(prefix), std::move(suffix), std::forward<F>(parser));
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
    static void
    add_infix_operator(
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
    static void
    add_infix_operator(
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
    static void
    add_infix_operator_overloads(
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
    static void
    add_infix_operator_for(
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
