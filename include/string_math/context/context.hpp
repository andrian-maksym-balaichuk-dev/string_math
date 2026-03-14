#pragma once

#include <array>
#include <set>

#include <string_math/internal/context_data.hpp>
#include <string_math/internal/parser.hpp>
#include <string_math/internal/static_context_data.hpp>
#include <string_math/semantics/overload.hpp>
#include <string_math/value/value.hpp>

namespace string_math
{

class MathContext;
class FrozenMathContext;
class Operation;
class Calculator;
class MathExpr;
MathValue evaluate_operation(const Operation& operation, const MathContext& context);

namespace internal
{

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
template <
    std::size_t VariableCount = 0,
    std::size_t InfixCount = 0,
    std::size_t FunctionCount = 0,
    std::size_t PrefixCount = 0,
    std::size_t PostfixCount = 0>
class StaticMathContext;
#endif

} // namespace internal

class MathContext
{
public:
    MathContext();
    MathContext(const MathContext& other);
    MathContext& operator=(const MathContext& other);
    MathContext(MathContext&&) noexcept = default;
    MathContext& operator=(MathContext&&) noexcept = default;
    explicit MathContext(const MathContext& parent, bool inherit_only);

    MathContext& set_parent(const MathContext& parent);
    MathContext with_parent(const MathContext& parent) const
    {
        MathContext copy(*this);
        copy.set_parent(parent);
        return copy;
    }
    bool remove_variable(std::string_view name);
    std::optional<MathValue> find_value(std::string_view name) const;
    FrozenMathContext snapshot() const;
    FrozenMathContext freeze() const;
    std::vector<std::string> variable_names() const;
    MathContext& set_policy(EvaluationPolicy policy)
    {
        m_data->m_policy = policy;
        return *this;
    }
    MathContext with_policy(EvaluationPolicy policy) const
    {
        MathContext copy(*this);
        copy.set_policy(policy);
        return copy;
    }
    const EvaluationPolicy& policy() const noexcept
    {
        return m_data->m_policy;
    }

    MathContext with_value(std::string_view name, MathValue value) const
    {
        return detail_with_value(name, value);
    }

    template <std::size_t NameSize, class T>
    MathContext with_value(const char (&name)[NameSize], T value) const
    {
        return with_value(std::string_view(name, NameSize - 1), MathValue(value));
    }

    MathContext& set_value(std::string_view name, MathValue value)
    {
        return detail_set_value(name, value);
    }

    template <std::size_t NameSize, class T>
    MathContext& set_value(const char (&name)[NameSize], T value)
    {
        return set_value(std::string_view(name, NameSize - 1), MathValue(value));
    }

    MathContext& add_variable(std::string_view name, MathValue value)
    {
        return detail_set_value(name, value);
    }

    template <std::size_t NameSize, class T>
    MathContext& add_variable(const char (&name)[NameSize], T value)
    {
        return add_variable(std::string_view(name, NameSize - 1), MathValue(value));
    }

    MathContext& register_variable(std::string_view name, MathValue value)
    {
        return detail_set_value(name, value);
    }

    template <std::size_t NameSize, class T>
    MathContext& register_variable(const char (&name)[NameSize], T value)
    {
        return register_variable(std::string_view(name, NameSize - 1), MathValue(value));
    }

    template <class Sig, class F>
    MathContext& add_function(std::string_view name, F&& function, CallableSemantics semantics = {})
    {
        auto& entry = m_data->m_functions[std::string(name)];
        internal::upsert_overload(
            entry.fixed_overloads,
            internal::make_function_overload<Sig>(std::forward<F>(function), semantics));
        return *this;
    }
    template <class F>
    MathContext& add_function(std::string_view name, F&& function, CallableSemantics semantics = {})
    {
        return add_function<fw::detail::fn_sig_t<F>>(name, std::forward<F>(function), semantics);
    }

    template <class Sig, class F>
    MathContext with_function(std::string_view name, F&& function, CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.template add_function<Sig>(name, std::forward<F>(function), semantics);
        return copy;
    }
    template <class F>
    MathContext with_function(std::string_view name, F&& function, CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.add_function(name, std::forward<F>(function), semantics);
        return copy;
    }

    template <class Sig, class F>
    MathContext& register_function(std::string_view name, F&& function, CallableSemantics semantics = {})
    {
        return add_function<Sig>(name, std::forward<F>(function), semantics);
    }
    template <class F>
    MathContext& register_function(std::string_view name, F&& function, CallableSemantics semantics = {})
    {
        return add_function(name, std::forward<F>(function), semantics);
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function>
    MathContext& add_function(std::string_view name, CallableSemantics semantics = {})
    {
        return detail_add_function_nttp<Sig, Function>(name, semantics);
    }

    template <auto Function>
    MathContext& add_function(std::string_view name, CallableSemantics semantics = {})
    {
        return add_function<fw::detail::fn_sig_t<decltype(Function)>, Function>(name, semantics);
    }

    template <class Sig, auto Function, std::size_t NameSize>
    MathContext& add_function(const char (&name)[NameSize], CallableSemantics semantics = {})
    {
        return add_function<Sig, Function>(std::string_view(name, NameSize - 1), semantics);
    }

    template <auto Function, std::size_t NameSize>
    MathContext& add_function(const char (&name)[NameSize], CallableSemantics semantics = {})
    {
        return add_function<Function>(std::string_view(name, NameSize - 1), semantics);
    }

    template <class Sig, auto Function>
    MathContext& register_function(std::string_view name, CallableSemantics semantics = {})
    {
        return add_function<Sig, Function>(name, semantics);
    }

    template <auto Function>
    MathContext& register_function(std::string_view name, CallableSemantics semantics = {})
    {
        return add_function<Function>(name, semantics);
    }
#endif

    template <class... Sigs, class F>
    MathContext& add_function_overloads(std::string_view name, F&& function, CallableSemantics semantics = {})
    {
        auto shared_function = std::decay_t<F>(std::forward<F>(function));
        (add_function<Sigs>(name, shared_function, semantics), ...);
        return *this;
    }

    template <class... Ts, class F>
    MathContext& add_function_for(std::string_view name, F&& function, CallableSemantics semantics = {})
    {
        return add_function_overloads<typename internal::unary_signature_for<Ts, std::decay_t<F>>::type...>(
            name, std::forward<F>(function), semantics);
    }

    template <class... Ts, class F>
    MathContext& add_binary_function_for(std::string_view name, F&& function, CallableSemantics semantics = {})
    {
        return add_function_overloads<typename internal::binary_signature_for<Ts, std::decay_t<F>>::type...>(
            name, std::forward<F>(function), semantics);
    }

    template <class F>
    MathContext& add_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {})
    {
        using wrapper_t = fw::function_wrapper<MathValue(const std::vector<MathValue>&)>;
        auto storage = std::make_shared<wrapper_t>(std::forward<F>(function));
        auto& entry = m_data->m_functions[std::string(name)];
        entry.variadic_overloads.push_back(typename internal::FunctionEntry::VariadicOverload{
            min_arity,
            semantics,
            storage,
            [](const void* raw_storage, const std::vector<MathValue>& arguments) {
                const auto& wrapper = *static_cast<const wrapper_t*>(raw_storage);
                return wrapper(arguments);
            },
        });
        return *this;
    }

    template <class T, class F>
    MathContext& add_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {})
    {
        auto typed_function = std::decay_t<F>(std::forward<F>(function));
        return add_variadic_function(
            name,
            min_arity,
            [typed_function](const std::vector<MathValue>& arguments) mutable -> MathValue {
                std::vector<T> converted;
                converted.reserve(arguments.size());
                for (const auto& argument : arguments)
                {
                    converted.push_back(argument.template cast<T>());
                }
                return MathValue(typed_function(converted));
            },
            semantics);
    }

    template <class F>
    MathContext& register_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {})
    {
        return add_variadic_function(name, min_arity, std::forward<F>(function), semantics);
    }

    template <class T, class F>
    MathContext& register_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {})
    {
        return add_variadic_function_for<T>(name, min_arity, std::forward<F>(function), semantics);
    }

    template <class F>
    MathContext with_variadic_function(
        std::string_view name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.add_variadic_function(name, min_arity, std::forward<F>(function), semantics);
        return copy;
    }

    template <class T, class F>
    MathContext with_variadic_function_for(
        std::string_view name,
        std::size_t min_arity,
        F&& function,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.template add_variadic_function_for<T>(name, min_arity, std::forward<F>(function), semantics);
        return copy;
    }

    bool remove_function(std::string_view name);

    template <class Sig, class F>
    MathContext&
    add_prefix_operator(std::string_view symbol, F&& function, int precedence = Precedence::Prefix, CallableSemantics semantics = {})
    {
        auto& entry = m_data->m_prefix_operators[std::string(symbol)];
        entry.precedence = precedence;
        internal::upsert_overload(entry.overloads, internal::make_unary_overload<Sig>(std::forward<F>(function), semantics));
        m_rebuild_symbol_cache();
        return *this;
    }
    template <class F>
    MathContext&
    add_prefix_operator(std::string_view symbol, F&& function, int precedence = Precedence::Prefix, CallableSemantics semantics = {})
    {
        return add_prefix_operator<fw::detail::fn_sig_t<F>>(symbol, std::forward<F>(function), precedence, semantics);
    }

    template <class Sig, class F>
    MathContext with_prefix_operator(
        std::string_view symbol,
        F&& function,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.template add_prefix_operator<Sig>(symbol, std::forward<F>(function), precedence, semantics);
        return copy;
    }

    template <class F>
    MathContext with_prefix_operator(
        std::string_view symbol,
        F&& function,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.add_prefix_operator(symbol, std::forward<F>(function), precedence, semantics);
        return copy;
    }

    template <class Sig, class F>
    MathContext& register_prefix_operator(
        std::string_view symbol,
        F&& function,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        return add_prefix_operator<Sig>(symbol, std::forward<F>(function), precedence, semantics);
    }
    template <class F>
    MathContext& register_prefix_operator(
        std::string_view symbol,
        F&& function,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        return add_prefix_operator(symbol, std::forward<F>(function), precedence, semantics);
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function>
    MathContext& add_prefix_operator(
        std::string_view symbol,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        return detail_add_prefix_operator_nttp<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function>
    MathContext& add_prefix_operator(
        std::string_view symbol,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        return add_prefix_operator<fw::detail::fn_sig_t<decltype(Function)>, Function>(symbol, precedence, semantics);
    }

    template <class Sig, auto Function>
    MathContext& register_prefix_operator(
        std::string_view symbol,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        return add_prefix_operator<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function>
    MathContext& register_prefix_operator(
        std::string_view symbol,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        return add_prefix_operator<Function>(symbol, precedence, semantics);
    }
#endif

    template <class... Sigs, class F>
    MathContext& add_prefix_operator_overloads(
        std::string_view symbol,
        F&& function,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        auto shared_function = std::decay_t<F>(std::forward<F>(function));
        (add_prefix_operator<Sigs>(symbol, shared_function, precedence, semantics), ...);
        return *this;
    }

    template <class... Ts, class F>
    MathContext& add_prefix_operator_for(
        std::string_view symbol,
        F&& function,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {})
    {
        return add_prefix_operator_overloads<typename internal::unary_signature_for<Ts, std::decay_t<F>>::type...>(
            symbol, std::forward<F>(function), precedence, semantics);
    }

    bool remove_prefix_operator(std::string_view symbol);

    template <class Sig, class F>
    MathContext&
    add_postfix_operator(std::string_view symbol, F&& function, int precedence = Precedence::Postfix, CallableSemantics semantics = {})
    {
        auto& entry = m_data->m_postfix_operators[std::string(symbol)];
        entry.precedence = precedence;
        internal::upsert_overload(entry.overloads, internal::make_unary_overload<Sig>(std::forward<F>(function), semantics));
        m_rebuild_symbol_cache();
        return *this;
    }
    template <class F>
    MathContext&
    add_postfix_operator(std::string_view symbol, F&& function, int precedence = Precedence::Postfix, CallableSemantics semantics = {})
    {
        return add_postfix_operator<fw::detail::fn_sig_t<F>>(symbol, std::forward<F>(function), precedence, semantics);
    }

    template <class Sig, class F>
    MathContext with_postfix_operator(
        std::string_view symbol,
        F&& function,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.template add_postfix_operator<Sig>(symbol, std::forward<F>(function), precedence, semantics);
        return copy;
    }

    template <class F>
    MathContext with_postfix_operator(
        std::string_view symbol,
        F&& function,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.add_postfix_operator(symbol, std::forward<F>(function), precedence, semantics);
        return copy;
    }

    template <class Sig, class F>
    MathContext& register_postfix_operator(
        std::string_view symbol,
        F&& function,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        return add_postfix_operator<Sig>(symbol, std::forward<F>(function), precedence, semantics);
    }
    template <class F>
    MathContext& register_postfix_operator(
        std::string_view symbol,
        F&& function,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        return add_postfix_operator(symbol, std::forward<F>(function), precedence, semantics);
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function>
    MathContext& add_postfix_operator(
        std::string_view symbol,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        return detail_add_postfix_operator_nttp<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function>
    MathContext& add_postfix_operator(
        std::string_view symbol,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        return add_postfix_operator<fw::detail::fn_sig_t<decltype(Function)>, Function>(symbol, precedence, semantics);
    }

    template <class Sig, auto Function>
    MathContext& register_postfix_operator(
        std::string_view symbol,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        return add_postfix_operator<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function>
    MathContext& register_postfix_operator(
        std::string_view symbol,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        return add_postfix_operator<Function>(symbol, precedence, semantics);
    }
#endif

    template <class... Sigs, class F>
    MathContext& add_postfix_operator_overloads(
        std::string_view symbol,
        F&& function,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        auto shared_function = std::decay_t<F>(std::forward<F>(function));
        (add_postfix_operator<Sigs>(symbol, shared_function, precedence, semantics), ...);
        return *this;
    }

    template <class... Ts, class F>
    MathContext& add_postfix_operator_for(
        std::string_view symbol,
        F&& function,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {})
    {
        return add_postfix_operator_overloads<typename internal::unary_signature_for<Ts, std::decay_t<F>>::type...>(
            symbol, std::forward<F>(function), precedence, semantics);
    }

    bool remove_postfix_operator(std::string_view symbol);

    template <class Sig, class F>
    MathContext&
    add_infix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        auto& entry = m_data->m_infix_operators[std::string(symbol)];
        entry.precedence = precedence;
        entry.associativity = associativity;
        const CallableSemantics effective_semantics =
            binary_policy == BinaryPolicyKind::None ? semantics : semantics.with_binary_policy(binary_policy);
        internal::upsert_overload(
            entry.overloads,
            internal::make_binary_overload<Sig>(std::forward<F>(function), effective_semantics));
        m_rebuild_symbol_cache();
        return *this;
    }
    template <class F>
    MathContext&
    add_infix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        return add_infix_operator<fw::detail::fn_sig_t<F>>(
            symbol, std::forward<F>(function), precedence, associativity, semantics, binary_policy);
    }

    template <class Sig, class F>
    MathContext with_infix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None) const
    {
        MathContext copy(*this);
        copy.template add_infix_operator<Sig>(
            symbol, std::forward<F>(function), precedence, associativity, semantics, binary_policy);
        return copy;
    }

    template <class F>
    MathContext with_infix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None) const
    {
        MathContext copy(*this);
        copy.add_infix_operator(symbol, std::forward<F>(function), precedence, associativity, semantics, binary_policy);
        return copy;
    }

    template <class Sig, class F>
    MathContext&
    register_infix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        return add_infix_operator<Sig>(
            symbol, std::forward<F>(function), precedence, associativity, semantics, binary_policy);
    }
    template <class F>
    MathContext&
    register_infix_operator(
        std::string_view symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        return add_infix_operator(symbol, std::forward<F>(function), precedence, associativity, semantics, binary_policy);
    }

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <class Sig, auto Function>
    MathContext& add_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        return detail_add_infix_operator_nttp<Sig, Function>(symbol, precedence, associativity, semantics, binary_policy);
    }

    template <auto Function>
    MathContext& add_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        return add_infix_operator<fw::detail::fn_sig_t<decltype(Function)>, Function>(
            symbol, precedence, associativity, semantics, binary_policy);
    }

    template <class Sig, auto Function>
    MathContext& register_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        return add_infix_operator<Sig, Function>(symbol, precedence, associativity, semantics, binary_policy);
    }

    template <auto Function>
    MathContext& register_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        return add_infix_operator<Function>(symbol, precedence, associativity, semantics, binary_policy);
    }
#endif

    template <class... Sigs, class F>
    MathContext&
    add_infix_operator_overloads(
        std::string_view symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        auto shared_function = std::decay_t<F>(std::forward<F>(function));
        (add_infix_operator<Sigs>(symbol, shared_function, precedence, associativity, semantics, binary_policy), ...);
        return *this;
    }

    template <class... Ts, class F>
    MathContext&
    add_infix_operator_for(
        std::string_view symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None)
    {
        return add_infix_operator_overloads<typename internal::binary_signature_for<Ts, std::decay_t<F>>::type...>(
            symbol, std::forward<F>(function), precedence, associativity, semantics, binary_policy);
    }

    template <class Sig, class F, class PolicyF>
    MathContext& add_infix_operator_with_policy(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        auto& entry = m_data->m_infix_operators[std::string(symbol)];
        entry.precedence = precedence;
        entry.associativity = associativity;
        internal::upsert_overload(
            entry.overloads,
            internal::make_binary_overload_with_policy<Sig>(
                std::forward<F>(function),
                std::forward<PolicyF>(policy_handler),
                semantics));
        m_rebuild_symbol_cache();
        return *this;
    }
#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <auto Function, auto PolicyHandler, std::size_t SymbolSize>
    MathContext& add_infix_operator_with_policy(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator_with_policy(
            std::string_view(symbol, SymbolSize - 1),
            Function,
            precedence,
            PolicyHandler,
            associativity,
            semantics);
    }
#endif

    template <class F, class PolicyF>
    MathContext& add_infix_operator_with_policy(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator_with_policy<fw::detail::fn_sig_t<F>>(
            symbol,
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
    }

    template <class Sig, class F, class PolicyF>
    MathContext& register_infix_operator_with_policy(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator_with_policy<Sig>(
            symbol,
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
    }
#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    template <auto Function, auto PolicyHandler, std::size_t SymbolSize>
    MathContext& register_infix_operator_with_policy(
        const char (&symbol)[SymbolSize],
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator_with_policy<Function, PolicyHandler>(symbol, precedence, associativity, semantics);
    }
#endif

    template <class F, class PolicyF>
    MathContext& register_infix_operator_with_policy(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {})
    {
        return add_infix_operator_with_policy(
            symbol,
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
    }

    template <class Sig, class F, class PolicyF>
    MathContext with_infix_operator_with_policy(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.template add_infix_operator_with_policy<Sig>(
            symbol,
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
        return copy;
    }

    template <class F, class PolicyF>
    MathContext with_infix_operator_with_policy(
        std::string_view symbol,
        F&& function,
        int precedence,
        PolicyF&& policy_handler,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        MathContext copy(*this);
        copy.add_infix_operator_with_policy(
            symbol,
            std::forward<F>(function),
            precedence,
            std::forward<PolicyF>(policy_handler),
            associativity,
            semantics);
        return copy;
    }

    bool remove_infix_operator(std::string_view symbol);

    template <class F>
    MathContext& add_literal_parser(std::string_view prefix, std::string_view suffix, F&& parser)
    {
        m_data->m_literal_parsers.push_back(
            internal::LiteralParserEntry{
                std::string(prefix),
                std::string(suffix),
                fw::function_wrapper<std::optional<MathValue>(std::string_view)>(std::forward<F>(parser)),
            });
        return *this;
    }

    template <class F>
    MathContext with_literal_parser(std::string_view prefix, std::string_view suffix, F&& parser) const
    {
        MathContext copy(*this);
        copy.add_literal_parser(prefix, suffix, std::forward<F>(parser));
        return copy;
    }

    template <class F>
    MathContext& register_literal_parser(std::string_view prefix, std::string_view suffix, F&& parser)
    {
        return add_literal_parser(prefix, suffix, std::forward<F>(parser));
    }

    void clear_literal_parsers();
    std::optional<MathValue> try_parse_literal(std::string_view token) const;
    const internal::PrefixOperatorEntry* find_prefix_operator(std::string_view symbol) const;
    const internal::PostfixOperatorEntry* find_postfix_operator(std::string_view symbol) const;
    const internal::InfixOperatorEntry* find_infix_operator(std::string_view symbol) const;
    const internal::FunctionEntry* find_function(std::string_view name) const;
    std::string match_prefix_symbol(std::string_view text) const;
    std::string match_postfix_symbol(std::string_view text) const;
    std::string match_infix_symbol(std::string_view text) const;
    std::vector<std::string> function_names() const;
    std::vector<std::string> prefix_operator_names() const;
    std::vector<std::string> postfix_operator_names() const;
    std::vector<std::string> infix_operator_names() const;
    std::vector<LiteralParserInfo> literal_parsers() const;
    std::optional<FunctionInfo> inspect_function(std::string_view name) const;
    std::optional<OperatorInfo> inspect_prefix_operator(std::string_view symbol) const;
    std::optional<OperatorInfo> inspect_postfix_operator(std::string_view symbol) const;
    std::optional<OperatorInfo> inspect_infix_operator(std::string_view symbol) const;
    static MathContext with_builtins();
#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
    static constexpr auto compile_time();
#endif

private:
    enum class SymbolKind
    {
        Prefix,
        Postfix,
        Infix
    };

    std::shared_ptr<internal::RuntimeContextData> m_data;

    MathContext& detail_set_value(std::string_view name, MathValue value);
    MathContext detail_with_value(std::string_view name, MathValue value) const
    {
        MathContext copy(*this);
        copy.detail_set_value(name, value);
        return copy;
    }
    template <class Sig, auto Function>
    MathContext& detail_add_function_nttp(std::string_view name, CallableSemantics semantics)
    {
        return add_function<Sig>(name, Function, semantics);
    }
    template <class Sig, auto Function>
    MathContext detail_add_function_nttp(std::string_view name, CallableSemantics semantics) const
    {
        return with_function<Sig>(name, Function, semantics);
    }
    template <class Sig, auto Function>
    MathContext& detail_add_prefix_operator_nttp(std::string_view symbol, int precedence, CallableSemantics semantics)
    {
        return add_prefix_operator<Sig>(symbol, Function, precedence, semantics);
    }
    template <class Sig, auto Function>
    MathContext detail_add_prefix_operator_nttp(std::string_view symbol, int precedence, CallableSemantics semantics) const
    {
        return with_prefix_operator<Sig>(symbol, Function, precedence, semantics);
    }
    template <class Sig, auto Function>
    MathContext& detail_add_postfix_operator_nttp(std::string_view symbol, int precedence, CallableSemantics semantics)
    {
        return add_postfix_operator<Sig>(symbol, Function, precedence, semantics);
    }
    template <class Sig, auto Function>
    MathContext detail_add_postfix_operator_nttp(std::string_view symbol, int precedence, CallableSemantics semantics) const
    {
        return with_postfix_operator<Sig>(symbol, Function, precedence, semantics);
    }
    template <class Sig, auto Function>
    MathContext& detail_add_infix_operator_nttp(
        std::string_view symbol,
        int precedence,
        Associativity associativity,
        CallableSemantics semantics,
        BinaryPolicyKind binary_policy)
    {
        return add_infix_operator<Sig>(symbol, Function, precedence, associativity, semantics, binary_policy);
    }
    template <class Sig, auto Function>
    MathContext detail_add_infix_operator_nttp(
        std::string_view symbol,
        int precedence,
        Associativity associativity,
        CallableSemantics semantics,
        BinaryPolicyKind binary_policy) const
    {
        return with_infix_operator<Sig>(symbol, Function, precedence, associativity, semantics, binary_policy);
    }

    std::string m_match_symbol(std::string_view text, SymbolKind kind) const;
    void m_rebuild_symbol_cache();
};

class FrozenMathContext
{
public:
    FrozenMathContext() = default;

    explicit FrozenMathContext(MathContext context) : m_context(std::move(context)) {}

    const MathContext& context() const noexcept
    {
        return m_context;
    }

    operator const MathContext&() const noexcept
    {
        return m_context;
    }

private:
    MathContext m_context;
};

inline MathContext::MathContext() : m_data(std::make_shared<internal::RuntimeContextData>()) {}

inline MathContext::MathContext(const MathContext& other) : m_data(std::make_shared<internal::RuntimeContextData>(*other.m_data)) {}

inline MathContext& MathContext::operator=(const MathContext& other)
{
    if (this != &other)
    {
        m_data = std::make_shared<internal::RuntimeContextData>(*other.m_data);
    }
    return *this;
}

inline MathContext::MathContext(const MathContext& parent, bool inherit_only) : m_data(std::make_shared<internal::RuntimeContextData>())
{
    (void)inherit_only;
    m_data->m_parent = parent.m_data;
}

inline MathContext& MathContext::set_parent(const MathContext& parent)
{
    m_data->m_parent = parent.m_data;
    return *this;
}

inline MathContext& MathContext::detail_set_value(std::string_view name, MathValue value)
{
    m_data->m_values[std::string(name)] = value;
    return *this;
}

inline bool MathContext::remove_variable(std::string_view name)
{
    return m_data->m_values.erase(std::string(name)) != 0;
}

inline std::optional<MathValue> MathContext::find_value(std::string_view name) const
{
    const std::string key(name);
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        const auto found = current->m_values.find(key);
        if (found != current->m_values.end())
        {
            return found->second;
        }
    }
    return std::nullopt;
}

inline FrozenMathContext MathContext::snapshot() const
{
    return FrozenMathContext(*this);
}

inline FrozenMathContext MathContext::freeze() const
{
    return snapshot();
}

inline std::vector<std::string> MathContext::variable_names() const
{
    std::set<std::string> names;
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& entry : current->m_values)
        {
            names.insert(entry.first);
        }
    }
    return std::vector<std::string>(names.begin(), names.end());
}

inline bool MathContext::remove_function(std::string_view name)
{
    return m_data->m_functions.erase(std::string(name)) != 0;
}

inline bool MathContext::remove_prefix_operator(std::string_view symbol)
{
    const bool removed = m_data->m_prefix_operators.erase(std::string(symbol)) != 0;
    if (removed)
    {
        m_rebuild_symbol_cache();
    }
    return removed;
}

inline bool MathContext::remove_postfix_operator(std::string_view symbol)
{
    const bool removed = m_data->m_postfix_operators.erase(std::string(symbol)) != 0;
    if (removed)
    {
        m_rebuild_symbol_cache();
    }
    return removed;
}

inline bool MathContext::remove_infix_operator(std::string_view symbol)
{
    const bool removed = m_data->m_infix_operators.erase(std::string(symbol)) != 0;
    if (removed)
    {
        m_rebuild_symbol_cache();
    }
    return removed;
}

inline void MathContext::clear_literal_parsers()
{
    m_data->m_literal_parsers.clear();
}

inline std::optional<MathValue> MathContext::try_parse_literal(std::string_view token) const
{
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& parser : current->m_literal_parsers)
        {
            if (!parser.prefix.empty() && !internal::starts_with(token, parser.prefix))
            {
                continue;
            }
            if (!parser.suffix.empty() && !internal::ends_with(token, parser.suffix))
            {
                continue;
            }
            if (auto value = parser.parser(token))
            {
                return value;
            }
        }
    }
    return std::nullopt;
}

inline const internal::PrefixOperatorEntry* MathContext::find_prefix_operator(std::string_view symbol) const
{
    const std::string key(symbol);
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        const auto found = current->m_prefix_operators.find(key);
        if (found != current->m_prefix_operators.end())
        {
            return &found->second;
        }
    }
    return nullptr;
}

inline const internal::PostfixOperatorEntry* MathContext::find_postfix_operator(std::string_view symbol) const
{
    const std::string key(symbol);
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        const auto found = current->m_postfix_operators.find(key);
        if (found != current->m_postfix_operators.end())
        {
            return &found->second;
        }
    }
    return nullptr;
}

inline const internal::InfixOperatorEntry* MathContext::find_infix_operator(std::string_view symbol) const
{
    const std::string key(symbol);
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        const auto found = current->m_infix_operators.find(key);
        if (found != current->m_infix_operators.end())
        {
            return &found->second;
        }
    }
    return nullptr;
}

inline const internal::FunctionEntry* MathContext::find_function(std::string_view name) const
{
    const std::string key(name);
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        const auto found = current->m_functions.find(key);
        if (found != current->m_functions.end())
        {
            return &found->second;
        }
    }
    return nullptr;
}

inline std::string MathContext::match_prefix_symbol(std::string_view text) const
{
    return m_match_symbol(text, SymbolKind::Prefix);
}

inline std::string MathContext::match_postfix_symbol(std::string_view text) const
{
    return m_match_symbol(text, SymbolKind::Postfix);
}

inline std::string MathContext::match_infix_symbol(std::string_view text) const
{
    return m_match_symbol(text, SymbolKind::Infix);
}

inline std::vector<std::string> MathContext::function_names() const
{
    std::set<std::string> names;
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& entry : current->m_functions)
        {
            names.insert(entry.first);
        }
    }
    return std::vector<std::string>(names.begin(), names.end());
}

inline std::vector<std::string> MathContext::prefix_operator_names() const
{
    std::set<std::string> names;
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& entry : current->m_prefix_operators)
        {
            names.insert(entry.first);
        }
    }
    return std::vector<std::string>(names.begin(), names.end());
}

inline std::vector<std::string> MathContext::postfix_operator_names() const
{
    std::set<std::string> names;
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& entry : current->m_postfix_operators)
        {
            names.insert(entry.first);
        }
    }
    return std::vector<std::string>(names.begin(), names.end());
}

inline std::vector<std::string> MathContext::infix_operator_names() const
{
    std::set<std::string> names;
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& entry : current->m_infix_operators)
        {
            names.insert(entry.first);
        }
    }
    return std::vector<std::string>(names.begin(), names.end());
}

inline std::vector<LiteralParserInfo> MathContext::literal_parsers() const
{
    std::vector<LiteralParserInfo> parsers;
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& parser : current->m_literal_parsers)
        {
            parsers.push_back(LiteralParserInfo{parser.prefix, parser.suffix});
        }
    }
    return parsers;
}

inline std::optional<FunctionInfo> MathContext::inspect_function(std::string_view name) const
{
    const auto* entry = find_function(name);
    if (entry == nullptr)
    {
        return std::nullopt;
    }

    FunctionInfo info;
    info.name = std::string(name);
    for (const auto& overload : entry->fixed_overloads)
    {
        info.fixed_overloads.push_back(FunctionOverloadInfo{
            overload.result_type,
            overload.argument_types,
            overload.semantics,
        });

        if (overload.argument_types.size() == 1)
        {
            info.unary_overloads.push_back(
                UnaryOverloadInfo{overload.result_type, overload.argument_types.front(), overload.semantics});
        }
        else if (overload.argument_types.size() == 2)
        {
            info.binary_overloads.push_back(BinaryOverloadInfo{
                overload.result_type,
                overload.argument_types[0],
                overload.argument_types[1],
                overload.semantics,
            });
        }
    }
    for (const auto& overload : entry->variadic_overloads)
    {
        info.variadic_min_arities.push_back(overload.min_arity);
        info.variadic_semantics.push_back(overload.semantics);
    }
    return info;
}

inline std::optional<OperatorInfo> MathContext::inspect_prefix_operator(std::string_view symbol) const
{
    const auto* entry = find_prefix_operator(symbol);
    if (entry == nullptr)
    {
        return std::nullopt;
    }

    OperatorInfo info;
    info.symbol = std::string(symbol);
    info.precedence = entry->precedence;
    for (const auto& overload : entry->overloads)
    {
        info.unary_overloads.push_back(UnaryOverloadInfo{overload.result_type, overload.argument_type, overload.semantics});
    }
    return info;
}

inline std::optional<OperatorInfo> MathContext::inspect_postfix_operator(std::string_view symbol) const
{
    const auto* entry = find_postfix_operator(symbol);
    if (entry == nullptr)
    {
        return std::nullopt;
    }

    OperatorInfo info;
    info.symbol = std::string(symbol);
    info.precedence = entry->precedence;
    for (const auto& overload : entry->overloads)
    {
        info.unary_overloads.push_back(UnaryOverloadInfo{overload.result_type, overload.argument_type, overload.semantics});
    }
    return info;
}

inline std::optional<OperatorInfo> MathContext::inspect_infix_operator(std::string_view symbol) const
{
    const auto* entry = find_infix_operator(symbol);
    if (entry == nullptr)
    {
        return std::nullopt;
    }

    OperatorInfo info;
    info.symbol = std::string(symbol);
    info.precedence = entry->precedence;
    info.associativity = entry->associativity;
    for (const auto& overload : entry->overloads)
    {
        info.binary_overloads.push_back(
            BinaryOverloadInfo{overload.result_type, overload.left_type, overload.right_type, overload.semantics});
    }
    return info;
}

inline std::string MathContext::m_match_symbol(std::string_view text, SymbolKind kind) const
{
    std::string best;
    for (const internal::RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        const auto* symbols = &current->m_prefix_symbols;
        if (kind == SymbolKind::Postfix)
        {
            symbols = &current->m_postfix_symbols;
        }
        else if (kind == SymbolKind::Infix)
        {
            symbols = &current->m_infix_symbols;
        }

        for (const auto& symbol : *symbols)
        {
            if (text.size() >= symbol.size() && text.substr(0, symbol.size()) == symbol && symbol.size() > best.size())
            {
                best = symbol;
            }
        }
    }

    return best;
}

inline void MathContext::m_rebuild_symbol_cache()
{
    auto rebuild = [](const auto& map) {
        std::vector<std::string> symbols;
        symbols.reserve(map.size());
        for (const auto& entry : map)
        {
            symbols.push_back(entry.first);
        }
        std::sort(symbols.begin(), symbols.end(), [](const std::string& left, const std::string& right) {
            if (left.size() != right.size())
            {
                return left.size() > right.size();
            }
            return left < right;
        });
        return symbols;
    };

    m_data->m_prefix_symbols = rebuild(m_data->m_prefix_operators);
    m_data->m_postfix_symbols = rebuild(m_data->m_postfix_operators);
    m_data->m_infix_symbols = rebuild(m_data->m_infix_operators);
}

inline Result<MathValue> try_parse_literal(std::string_view token, const MathContext& context, SourceSpan span = {})
{
    try
    {
        if (auto custom = context.try_parse_literal(token))
        {
            return *custom;
        }
    }
    catch (const std::exception& error)
    {
        return Error(ErrorKind::Literal, error.what(), span, std::string(token));
    }

    try
    {
        return internal::parse_builtin_literal(token);
    }
    catch (const std::exception& error)
    {
        return Error(ErrorKind::Literal, error.what(), span, std::string(token));
    }
}

inline MathValue parse_literal(std::string_view token, const MathContext& context)
{
    return try_parse_literal(token, context).value();
}

} // namespace string_math
