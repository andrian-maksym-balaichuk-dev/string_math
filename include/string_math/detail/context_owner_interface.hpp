#pragma once

#include <utility>

#include <string_math/context.hpp>

namespace string_math::detail
{

template <class Derived>
class ContextOwnerInterface
{
public:
    Derived& set_value(std::string name, MathValue value)
    {
        m_context().set_value(std::move(name), value);
        return m_self();
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <std::size_t NameSize, class T>
    Derived& set_value(const char (&name)[NameSize], T value)
    {
        m_context().set_value(name, value);
        return m_self();
    }
#endif

    Derived& add_variable(std::string name, MathValue value)
    {
        m_context().add_variable(std::move(name), value);
        return m_self();
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <std::size_t NameSize, class T>
    Derived& add_variable(const char (&name)[NameSize], T value)
    {
        m_context().add_variable(name, value);
        return m_self();
    }
#endif

    Derived& register_variable(std::string name, MathValue value)
    {
        return add_variable(std::move(name), value);
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <std::size_t NameSize, class T>
    Derived& register_variable(const char (&name)[NameSize], T value)
    {
        return add_variable(name, value);
    }
#endif

    bool remove_variable(const std::string& name)
    {
        return m_context().remove_variable(name);
    }

    template <class Sig, class F>
    Derived& add_function(std::string name, F&& function, CallableSemantics semantics = {})
    {
        m_context().template add_function<Sig>(std::move(name), std::forward<F>(function), semantics);
        return m_self();
    }

    template <class F>
    Derived& add_function(std::string name, F&& function, CallableSemantics semantics = {})
    {
        m_context().add_function(std::move(name), std::forward<F>(function), semantics);
        return m_self();
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function, std::size_t NameSize>
    Derived& add_function(const char (&name)[NameSize], CallableSemantics semantics = {})
    {
        m_context().template add_function<Sig, Function>(name, semantics);
        return m_self();
    }

    template <auto Function, std::size_t NameSize>
    Derived& add_function(const char (&name)[NameSize], CallableSemantics semantics = {})
    {
        m_context().template add_function<Function>(name, semantics);
        return m_self();
    }
#endif

    template <class Sig, class F>
    Derived& register_function(std::string name, F&& function, CallableSemantics semantics = {})
    {
        return add_function<Sig>(std::move(name), std::forward<F>(function), semantics);
    }

    template <class F>
    Derived& register_function(std::string name, F&& function, CallableSemantics semantics = {})
    {
        return add_function(std::move(name), std::forward<F>(function), semantics);
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function, std::size_t NameSize>
    Derived& register_function(const char (&name)[NameSize], CallableSemantics semantics = {})
    {
        return add_function<Sig, Function>(name, semantics);
    }

    template <auto Function, std::size_t NameSize>
    Derived& register_function(const char (&name)[NameSize], CallableSemantics semantics = {})
    {
        return add_function<Function>(name, semantics);
    }
#endif

    template <class... Sigs, class F>
    Derived& add_function_overloads(std::string name, F&& function, CallableSemantics semantics = {})
    {
        m_context().template add_function_overloads<Sigs...>(std::move(name), std::forward<F>(function), semantics);
        return m_self();
    }

    template <class... Ts, class F>
    Derived& add_function_for(std::string name, F&& function, CallableSemantics semantics = {})
    {
        m_context().template add_function_for<Ts...>(std::move(name), std::forward<F>(function), semantics);
        return m_self();
    }

    template <class... Ts, class F>
    Derived& add_binary_function_for(std::string name, F&& function, CallableSemantics semantics = {})
    {
        m_context().template add_binary_function_for<Ts...>(std::move(name), std::forward<F>(function), semantics);
        return m_self();
    }

    template <class F>
    Derived& add_variadic_function(
        std::string name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {})
    {
        m_context().add_variadic_function(std::move(name), min_arity, std::forward<F>(function), semantics);
        return m_self();
    }

    template <class T, class F>
    Derived& add_variadic_function_for(
        std::string name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {})
    {
        m_context().template add_variadic_function_for<T>(std::move(name), min_arity, std::forward<F>(function), semantics);
        return m_self();
    }

    template <class F>
    Derived& register_variadic_function(
        std::string name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {})
    {
        return add_variadic_function(std::move(name), min_arity, std::forward<F>(function), semantics);
    }

    template <class T, class F>
    Derived& register_variadic_function_for(
        std::string name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {})
    {
        return add_variadic_function_for<T>(std::move(name), min_arity, std::forward<F>(function), semantics);
    }

    bool remove_function(const std::string& name)
    {
        return m_context().remove_function(name);
    }

    template <class Sig, class F>
    Derived& add_prefix_operator(
        std::string symbol,
        F&& function,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        m_context().template add_prefix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence, semantics);
        return m_self();
    }

    template <class F>
    Derived& add_prefix_operator(
        std::string symbol,
        F&& function,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        m_context().add_prefix_operator(std::move(symbol), std::forward<F>(function), precedence, semantics);
        return m_self();
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function, std::size_t SymbolSize>
    Derived& add_prefix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        m_context().template add_prefix_operator<Sig, Function>(symbol, precedence, semantics);
        return m_self();
    }

    template <auto Function, std::size_t SymbolSize>
    Derived& add_prefix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        m_context().template add_prefix_operator<Function>(symbol, precedence, semantics);
        return m_self();
    }
#endif

    template <class Sig, class F>
    Derived& register_prefix_operator(
        std::string symbol,
        F&& function,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        return add_prefix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence, semantics);
    }

    template <class F>
    Derived& register_prefix_operator(
        std::string symbol,
        F&& function,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        return add_prefix_operator(std::move(symbol), std::forward<F>(function), precedence, semantics);
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function, std::size_t SymbolSize>
    Derived& register_prefix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        return add_prefix_operator<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function, std::size_t SymbolSize>
    Derived& register_prefix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        return add_prefix_operator<Function>(symbol, precedence, semantics);
    }
#endif

    template <class... Sigs, class F>
    Derived& add_prefix_operator_overloads(
        std::string symbol,
        F&& function,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        m_context().template add_prefix_operator_overloads<Sigs...>(std::move(symbol), std::forward<F>(function), precedence, semantics);
        return m_self();
    }

    template <class... Ts, class F>
    Derived& add_prefix_operator_for(
        std::string symbol,
        F&& function,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        m_context().template add_prefix_operator_for<Ts...>(std::move(symbol), std::forward<F>(function), precedence, semantics);
        return m_self();
    }

    template <class Sig, class F>
    Derived& add_postfix_operator(
        std::string symbol,
        F&& function,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        m_context().template add_postfix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence, semantics);
        return m_self();
    }

    template <class F>
    Derived& add_postfix_operator(
        std::string symbol,
        F&& function,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        m_context().add_postfix_operator(std::move(symbol), std::forward<F>(function), precedence, semantics);
        return m_self();
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function, std::size_t SymbolSize>
    Derived& add_postfix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        m_context().template add_postfix_operator<Sig, Function>(symbol, precedence, semantics);
        return m_self();
    }

    template <auto Function, std::size_t SymbolSize>
    Derived& add_postfix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        m_context().template add_postfix_operator<Function>(symbol, precedence, semantics);
        return m_self();
    }
#endif

    template <class Sig, class F>
    Derived& register_postfix_operator(
        std::string symbol,
        F&& function,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        return add_postfix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence, semantics);
    }

    template <class F>
    Derived& register_postfix_operator(
        std::string symbol,
        F&& function,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        return add_postfix_operator(std::move(symbol), std::forward<F>(function), precedence, semantics);
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function, std::size_t SymbolSize>
    Derived& register_postfix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        return add_postfix_operator<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function, std::size_t SymbolSize>
    Derived& register_postfix_operator(
        const char (&symbol)[SymbolSize],
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        return add_postfix_operator<Function>(symbol, precedence, semantics);
    }
#endif

    template <class... Sigs, class F>
    Derived& add_postfix_operator_overloads(
        std::string symbol,
        F&& function,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        m_context().template add_postfix_operator_overloads<Sigs...>(std::move(symbol), std::forward<F>(function), precedence, semantics);
        return m_self();
    }

    template <class... Ts, class F>
    Derived& add_postfix_operator_for(
        std::string symbol,
        F&& function,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        m_context().template add_postfix_operator_for<Ts...>(std::move(symbol), std::forward<F>(function), precedence, semantics);
        return m_self();
    }

    template <class Sig, class F>
    Derived& add_infix_operator(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        m_context().template add_infix_operator<Sig>(
            std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
        return m_self();
    }

    template <class F>
    Derived& add_infix_operator(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        m_context().add_infix_operator(
            std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
        return m_self();
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function, std::size_t SymbolSize>
    Derived& add_infix_operator(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        m_context().template add_infix_operator<Sig, Function>(
            symbol, precedence, associativity, semantics, binary_policy);
        return m_self();
    }

    template <auto Function, std::size_t SymbolSize>
    Derived& add_infix_operator(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        m_context().template add_infix_operator<Function>(
            symbol, precedence, associativity, semantics, binary_policy);
        return m_self();
    }
#endif

    template <class Sig, class F>
    Derived& register_infix_operator(
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
    Derived& register_infix_operator(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        return add_infix_operator(
            std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function, std::size_t SymbolSize>
    Derived& register_infix_operator(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        return add_infix_operator<Sig, Function>(symbol, precedence, associativity, semantics, binary_policy);
    }

    template <auto Function, std::size_t SymbolSize>
    Derived& register_infix_operator(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        return add_infix_operator<Function>(symbol, precedence, associativity, semantics, binary_policy);
    }
#endif

    template <class... Sigs, class F>
    Derived& add_infix_operator_overloads(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        m_context().template add_infix_operator_overloads<Sigs...>(
            std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
        return m_self();
    }

    template <class... Ts, class F>
    Derived& add_infix_operator_for(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        m_context().template add_infix_operator_for<Ts...>(
            std::move(symbol), std::forward<F>(function), precedence, associativity, semantics, binary_policy);
        return m_self();
    }

    template <class Sig, class F, class PolicyF>
    Derived& add_infix_operator_with_policy(
        std::string symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        m_context().template add_infix_operator_with_policy<Sig>(
            std::move(symbol),
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
        return m_self();
    }

    template <class F, class PolicyF>
    Derived& add_infix_operator_with_policy(
        std::string symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        m_context().add_infix_operator_with_policy(
            std::move(symbol),
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
        return m_self();
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <auto Function, auto PolicyHandler, std::size_t SymbolSize>
    Derived& add_infix_operator_with_policy(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        m_context().template add_infix_operator_with_policy<Function, PolicyHandler>(symbol, precedence, associativity, semantics);
        return m_self();
    }
#endif

    template <class Sig, class F, class PolicyF>
    Derived& register_infix_operator_with_policy(
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
    Derived& register_infix_operator_with_policy(
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
    Derived& register_infix_operator_with_policy(
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
        return m_context().remove_prefix_operator(symbol);
    }

    bool remove_postfix_operator(const std::string& symbol)
    {
        return m_context().remove_postfix_operator(symbol);
    }

    bool remove_infix_operator(const std::string& symbol)
    {
        return m_context().remove_infix_operator(symbol);
    }

    template <class F>
    Derived& add_literal_parser(std::string prefix, std::string suffix, F&& parser)
    {
        m_context().add_literal_parser(std::move(prefix), std::move(suffix), std::forward<F>(parser));
        return m_self();
    }

    template <class F>
    Derived& register_literal_parser(std::string prefix, std::string suffix, F&& parser)
    {
        return add_literal_parser(std::move(prefix), std::move(suffix), std::forward<F>(parser));
    }

protected:
    ~ContextOwnerInterface() = default;

private:
    Derived& m_self()
    {
        return static_cast<Derived&>(*this);
    }

    MathContext& m_context()
    {
        return m_self().mutable_context();
    }
};

} // namespace string_math::detail
