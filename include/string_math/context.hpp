#pragma once

#include <array>
#include <set>

#include <string_math/detail/literal.hpp>
#include <string_math/detail/overload.hpp>
#include <string_math/policy.hpp>
#include <string_math/result.hpp>
#include <string_math/semantics.hpp>
#include <string_math/value.hpp>

namespace string_math
{

class MathContext;
class FrozenMathContext;
class Operation;
class Calculator;
class MathExpr;
MathValue evaluate_operation(const Operation& operation, const MathContext& context);

struct UnaryOverloadInfo
{
    ValueType result_type{};
    ValueType argument_type{};
    CallableSemantics semantics{};
};

struct BinaryOverloadInfo
{
    ValueType result_type{};
    ValueType left_type{};
    ValueType right_type{};
    CallableSemantics semantics{};
};

struct FunctionInfo
{
    std::string name;
    std::vector<UnaryOverloadInfo> unary_overloads;
    std::vector<BinaryOverloadInfo> binary_overloads;
    std::vector<std::size_t> variadic_min_arities;
    std::vector<CallableSemantics> variadic_semantics;
};

struct OperatorInfo
{
    std::string symbol;
    int precedence{0};
    Associativity associativity{Associativity::Left};
    std::vector<UnaryOverloadInfo> unary_overloads;
    std::vector<BinaryOverloadInfo> binary_overloads;
};

struct LiteralParserInfo
{
    std::string prefix;
    std::string suffix;
};

namespace detail
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

struct LiteralParserEntry
{
    std::string prefix;
    std::string suffix;
    fw::function_wrapper<std::optional<MathValue>(std::string_view)> parser;
};

struct PrefixOperatorEntry
{
    int precedence{ Precedence::Prefix };
    std::vector<UnaryOverload> overloads;
};

struct PostfixOperatorEntry
{
    int precedence{ Precedence::Postfix };
    std::vector<UnaryOverload> overloads;
};

struct InfixOperatorEntry
{
    int precedence{ Precedence::Additive };
    Associativity associativity{ Associativity::Left };
    std::vector<BinaryOverload> overloads;
};

struct FunctionEntry
{
    std::vector<UnaryOverload> unary_overloads;
    std::vector<BinaryOverload> binary_overloads;
    struct VariadicOverload
    {
        std::size_t min_arity{1};
        CallableSemantics semantics{};
        std::shared_ptr<const void> storage;
        MathValue (*invoke)(const void*, const std::vector<MathValue>&) = nullptr;
    };
    std::vector<VariadicOverload> variadic_overloads;
};

template <class T, class F>
struct unary_signature_for
{
    using result_type = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F&, T>>>;
    static_assert(is_supported_value_type_v<result_type>, "unsupported unary helper result type");
    using type = result_type(T);
};

template <class T, class F>
struct binary_signature_for
{
    using result_type = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F&, T, T>>>;
    static_assert(is_supported_value_type_v<result_type>, "unsupported binary helper result type");
    using type = result_type(T, T);
};

} // namespace detail

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
        using traits = detail::signature_traits<Sig>;
        auto& entry = m_data->m_functions[std::string(name)];
        if constexpr (traits::arity == 1)
        {
            detail::upsert_overload(entry.unary_overloads, detail::make_unary_overload<Sig>(std::forward<F>(function), semantics));
        }
        else if constexpr (traits::arity == 2)
        {
            detail::upsert_overload(entry.binary_overloads, detail::make_binary_overload<Sig>(std::forward<F>(function), semantics));
        }
        else
        {
            static_assert(detail::always_false_v<Sig>, "functions support arity 1 or 2 only");
        }
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
        return add_function_overloads<typename detail::unary_signature_for<Ts, std::decay_t<F>>::type...>(
            name, std::forward<F>(function), semantics);
    }

    template <class... Ts, class F>
    MathContext& add_binary_function_for(std::string_view name, F&& function, CallableSemantics semantics = {})
    {
        return add_function_overloads<typename detail::binary_signature_for<Ts, std::decay_t<F>>::type...>(
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
        entry.variadic_overloads.push_back(typename detail::FunctionEntry::VariadicOverload{
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
        detail::upsert_overload(entry.overloads, detail::make_unary_overload<Sig>(std::forward<F>(function), semantics));
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
        return add_prefix_operator_overloads<typename detail::unary_signature_for<Ts, std::decay_t<F>>::type...>(
            symbol, std::forward<F>(function), precedence, semantics);
    }

    bool remove_prefix_operator(std::string_view symbol);

    template <class Sig, class F>
    MathContext&
    add_postfix_operator(std::string_view symbol, F&& function, int precedence = Precedence::Postfix, CallableSemantics semantics = {})
    {
        auto& entry = m_data->m_postfix_operators[std::string(symbol)];
        entry.precedence = precedence;
        detail::upsert_overload(entry.overloads, detail::make_unary_overload<Sig>(std::forward<F>(function), semantics));
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
        return add_postfix_operator_overloads<typename detail::unary_signature_for<Ts, std::decay_t<F>>::type...>(
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
        detail::upsert_overload(
            entry.overloads,
            detail::make_binary_overload<Sig>(std::forward<F>(function), effective_semantics));
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
        return add_infix_operator_overloads<typename detail::binary_signature_for<Ts, std::decay_t<F>>::type...>(
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
        detail::upsert_overload(
            entry.overloads,
            detail::make_binary_overload_with_policy<Sig>(
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
            detail::LiteralParserEntry{
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
    const detail::PrefixOperatorEntry* find_prefix_operator(std::string_view symbol) const;
    const detail::PostfixOperatorEntry* find_postfix_operator(std::string_view symbol) const;
    const detail::InfixOperatorEntry* find_infix_operator(std::string_view symbol) const;
    const detail::FunctionEntry* find_function(std::string_view name) const;
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

    struct RuntimeContextData
    {
        std::shared_ptr<RuntimeContextData> m_parent;
        std::unordered_map<std::string, MathValue> m_values;
        std::unordered_map<std::string, detail::FunctionEntry> m_functions;
        std::unordered_map<std::string, detail::PrefixOperatorEntry> m_prefix_operators;
        std::unordered_map<std::string, detail::PostfixOperatorEntry> m_postfix_operators;
        std::unordered_map<std::string, detail::InfixOperatorEntry> m_infix_operators;
        std::vector<detail::LiteralParserEntry> m_literal_parsers;
        std::vector<std::string> m_prefix_symbols;
        std::vector<std::string> m_postfix_symbols;
        std::vector<std::string> m_infix_symbols;
        EvaluationPolicy m_policy{};
    };

    std::shared_ptr<RuntimeContextData> m_data;

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

inline MathContext::MathContext() : m_data(std::make_shared<RuntimeContextData>()) {}

inline MathContext::MathContext(const MathContext& other) : m_data(std::make_shared<RuntimeContextData>(*other.m_data)) {}

inline MathContext& MathContext::operator=(const MathContext& other)
{
    if (this != &other)
    {
        m_data = std::make_shared<RuntimeContextData>(*other.m_data);
    }
    return *this;
}

inline MathContext::MathContext(const MathContext& parent, bool inherit_only) : m_data(std::make_shared<RuntimeContextData>())
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
    for (const RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
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
    for (const RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
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
    for (const RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& parser : current->m_literal_parsers)
        {
            if (!parser.prefix.empty() && !detail::starts_with(token, parser.prefix))
            {
                continue;
            }
            if (!parser.suffix.empty() && !detail::ends_with(token, parser.suffix))
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

inline const detail::PrefixOperatorEntry* MathContext::find_prefix_operator(std::string_view symbol) const
{
    const std::string key(symbol);
    for (const RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        const auto found = current->m_prefix_operators.find(key);
        if (found != current->m_prefix_operators.end())
        {
            return &found->second;
        }
    }
    return nullptr;
}

inline const detail::PostfixOperatorEntry* MathContext::find_postfix_operator(std::string_view symbol) const
{
    const std::string key(symbol);
    for (const RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        const auto found = current->m_postfix_operators.find(key);
        if (found != current->m_postfix_operators.end())
        {
            return &found->second;
        }
    }
    return nullptr;
}

inline const detail::InfixOperatorEntry* MathContext::find_infix_operator(std::string_view symbol) const
{
    const std::string key(symbol);
    for (const RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        const auto found = current->m_infix_operators.find(key);
        if (found != current->m_infix_operators.end())
        {
            return &found->second;
        }
    }
    return nullptr;
}

inline const detail::FunctionEntry* MathContext::find_function(std::string_view name) const
{
    const std::string key(name);
    for (const RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
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
    for (const RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
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
    for (const RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
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
    for (const RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
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
    for (const RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
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
    for (const RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
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
    for (const auto& overload : entry->unary_overloads)
    {
        info.unary_overloads.push_back(UnaryOverloadInfo{overload.result_type, overload.argument_type, overload.semantics});
    }
    for (const auto& overload : entry->binary_overloads)
    {
        info.binary_overloads.push_back(
            BinaryOverloadInfo{overload.result_type, overload.left_type, overload.right_type, overload.semantics});
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
    for (const RuntimeContextData* current = m_data.get(); current != nullptr; current = current->m_parent.get())
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
        return parse_builtin_literal(token);
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

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
namespace detail
{

struct ConstexprVariableEntry
{
    std::string_view name;
    MathValue value;
};

struct ConstexprInfixOperatorEntry
{
    std::string_view symbol;
    int precedence{0};
    Associativity associativity{Associativity::Left};
    fw::static_function_ref<MathValue(const MathValue&, const MathValue&)> invoke{};
    CallableSemantics semantics{};
    bool m_uses_typed_callable{false};
    ValueType m_left_type{ValueType::Int};
    ValueType m_right_type{ValueType::Int};
    fw::static_function<STRING_MATH_ALL_BINARY_SAME_TYPE_SIGNATURES> m_typed_invoke{};
};

struct ConstexprPrefixOperatorEntry
{
    std::string_view symbol;
    int precedence{Precedence::Prefix};
    fw::static_function_ref<MathValue(const MathValue&)> invoke{};
    CallableSemantics semantics{};
    bool m_uses_typed_callable{false};
    ValueType m_argument_type{ValueType::Int};
    fw::static_function<STRING_MATH_ALL_UNARY_SAME_TYPE_SIGNATURES> m_typed_invoke{};
};

struct ConstexprPostfixOperatorEntry
{
    std::string_view symbol;
    int precedence{Precedence::Postfix};
    fw::static_function_ref<MathValue(const MathValue&)> invoke{};
    CallableSemantics semantics{};
    bool m_uses_typed_callable{false};
    ValueType m_argument_type{ValueType::Int};
    fw::static_function<STRING_MATH_ALL_UNARY_SAME_TYPE_SIGNATURES> m_typed_invoke{};
};

struct ConstexprFunctionEntry
{
    std::string_view name;
    fw::static_function_ref<MathValue(const MathValue*, std::size_t)> invoke{};
    CallableSemantics semantics{};
    std::size_t m_arity{0};
    bool m_uses_typed_callable{false};
    ValueType m_first_type{ValueType::Int};
    ValueType m_second_type{ValueType::Int};
    fw::static_function<STRING_MATH_ALL_UNARY_SAME_TYPE_SIGNATURES> m_unary_typed_invoke{};
    fw::static_function<STRING_MATH_ALL_BINARY_SAME_TYPE_SIGNATURES> m_binary_typed_invoke{};
};

template <class Callable>
constexpr MathValue invoke_constexpr_unary_typed(const Callable& callable, ValueType argument_type, const MathValue& argument)
{
    switch (argument_type)
    {
    case ValueType::Short: return MathValue(callable(argument.template cast<short>()));
    case ValueType::UnsignedShort: return MathValue(callable(argument.template cast<unsigned short>()));
    case ValueType::Int: return MathValue(callable(argument.template cast<int>()));
    case ValueType::UnsignedInt: return MathValue(callable(argument.template cast<unsigned int>()));
    case ValueType::Long: return MathValue(callable(argument.template cast<long>()));
    case ValueType::UnsignedLong: return MathValue(callable(argument.template cast<unsigned long>()));
    case ValueType::LongLong: return MathValue(callable(argument.template cast<long long>()));
    case ValueType::UnsignedLongLong: return MathValue(callable(argument.template cast<unsigned long long>()));
    case ValueType::Float: return MathValue(callable(argument.template cast<float>()));
    case ValueType::Double: return MathValue(callable(argument.template cast<double>()));
    case ValueType::LongDouble: return MathValue(callable(argument.template cast<long double>()));
    }

    throw std::invalid_argument("string_math: unsupported constexpr unary callable type");
}

template <class Callable>
constexpr MathValue invoke_constexpr_binary_typed(
    const Callable& callable,
    ValueType left_type,
    ValueType right_type,
    const MathValue& left,
    const MathValue& right)
{
    if (left_type != right_type)
    {
        throw std::invalid_argument("string_math: constexpr typed binary callable requires matching argument types");
    }

    switch (left_type)
    {
    case ValueType::Short: return MathValue(callable(left.template cast<short>(), right.template cast<short>()));
    case ValueType::UnsignedShort:
        return MathValue(callable(left.template cast<unsigned short>(), right.template cast<unsigned short>()));
    case ValueType::Int: return MathValue(callable(left.template cast<int>(), right.template cast<int>()));
    case ValueType::UnsignedInt:
        return MathValue(callable(left.template cast<unsigned int>(), right.template cast<unsigned int>()));
    case ValueType::Long: return MathValue(callable(left.template cast<long>(), right.template cast<long>()));
    case ValueType::UnsignedLong:
        return MathValue(callable(left.template cast<unsigned long>(), right.template cast<unsigned long>()));
    case ValueType::LongLong:
        return MathValue(callable(left.template cast<long long>(), right.template cast<long long>()));
    case ValueType::UnsignedLongLong:
        return MathValue(callable(left.template cast<unsigned long long>(), right.template cast<unsigned long long>()));
    case ValueType::Float: return MathValue(callable(left.template cast<float>(), right.template cast<float>()));
    case ValueType::Double: return MathValue(callable(left.template cast<double>(), right.template cast<double>()));
    case ValueType::LongDouble:
        return MathValue(callable(left.template cast<long double>(), right.template cast<long double>()));
    }

    throw std::invalid_argument("string_math: unsupported constexpr binary callable type");
}

constexpr MathValue invoke_constexpr_prefix(const ConstexprPrefixOperatorEntry& entry, const MathValue& value)
{
    if (entry.m_uses_typed_callable)
    {
        return invoke_constexpr_unary_typed(entry.m_typed_invoke, entry.m_argument_type, value);
    }
    return entry.invoke(value);
}

constexpr MathValue invoke_constexpr_postfix(const ConstexprPostfixOperatorEntry& entry, const MathValue& value)
{
    if (entry.m_uses_typed_callable)
    {
        return invoke_constexpr_unary_typed(entry.m_typed_invoke, entry.m_argument_type, value);
    }
    return entry.invoke(value);
}

constexpr MathValue invoke_constexpr_infix(
    const ConstexprInfixOperatorEntry& entry,
    const MathValue& left,
    const MathValue& right)
{
    if (entry.m_uses_typed_callable)
    {
        return invoke_constexpr_binary_typed(entry.m_typed_invoke, entry.m_left_type, entry.m_right_type, left, right);
    }
    return entry.invoke(left, right);
}

constexpr MathValue invoke_constexpr_function(
    const ConstexprFunctionEntry& entry,
    const MathValue* arguments,
    std::size_t count)
{
    if (entry.m_uses_typed_callable)
    {
        if (count != entry.m_arity)
        {
            throw std::invalid_argument("string_math: constexpr function argument count mismatch");
        }
        if (entry.m_arity == 1)
        {
            return invoke_constexpr_unary_typed(entry.m_unary_typed_invoke, entry.m_first_type, arguments[0]);
        }
        return invoke_constexpr_binary_typed(
            entry.m_binary_typed_invoke,
            entry.m_first_type,
            entry.m_second_type,
            arguments[0],
            arguments[1]);
    }
    return entry.invoke(arguments, count);
}

template <
    std::size_t VariableCount,
    std::size_t InfixCount,
    std::size_t FunctionCount,
    std::size_t PrefixCount,
    std::size_t PostfixCount>
struct StaticContextData
{
    std::array<ConstexprVariableEntry, VariableCount> m_values{};
    std::array<ConstexprInfixOperatorEntry, InfixCount> m_infix_operators{};
    std::array<ConstexprFunctionEntry, FunctionCount> m_functions{};
    std::array<ConstexprPrefixOperatorEntry, PrefixCount> m_prefix_operators{};
    std::array<ConstexprPostfixOperatorEntry, PostfixCount> m_postfix_operators{};
    EvaluationPolicy m_policy{};
};

template <
    std::size_t VariableCount,
    std::size_t InfixCount,
    std::size_t FunctionCount,
    std::size_t PrefixCount,
    std::size_t PostfixCount>
class StaticMathContext
{
public:
    static constexpr bool is_constexpr_context = true;

    constexpr StaticMathContext() = default;

    constexpr auto with_value(std::string_view name, MathValue value) const
    {
        return detail_set_value(name, value);
    }

    template <std::size_t NameSize, class T>
    constexpr auto with_value(const char (&name)[NameSize], T value) const
    {
        return with_value(std::string_view(name, NameSize - 1), MathValue(value));
    }

    constexpr auto set_value(std::string_view name, MathValue value) const
    {
        return detail_set_value(name, value);
    }

    template <std::size_t NameSize, class T>
    constexpr auto set_value(const char (&name)[NameSize], T value) const
    {
        return set_value(std::string_view(name, NameSize - 1), MathValue(value));
    }

    constexpr auto add_variable(std::string_view name, MathValue value) const
    {
        return detail_set_value(name, value);
    }

    template <std::size_t NameSize, class T>
    constexpr auto add_variable(const char (&name)[NameSize], T value) const
    {
        return add_variable(std::string_view(name, NameSize - 1), MathValue(value));
    }

    constexpr auto register_variable(std::string_view name, MathValue value) const
    {
        return detail_set_value(name, value);
    }

    template <std::size_t NameSize, class T>
    constexpr auto register_variable(const char (&name)[NameSize], T value) const
    {
        return register_variable(std::string_view(name, NameSize - 1), MathValue(value));
    }

    constexpr auto detail_with_value_entry(std::string_view name, MathValue value) const
    {
        return detail_set_value(name, value);
    }

    constexpr auto detail_with_infix_entry(
        std::string_view symbol,
        int precedence,
        Associativity associativity,
        fw::static_function_ref<MathValue(const MathValue&, const MathValue&)> invoke,
        CallableSemantics semantics = {}) const
    {
        return m_with_infix_operator(symbol, precedence, associativity, invoke, semantics);
    }

    constexpr auto detail_with_prefix_entry(
        std::string_view symbol,
        int precedence,
        fw::static_function_ref<MathValue(const MathValue&)> invoke,
        CallableSemantics semantics = {}) const
    {
        return m_with_prefix_operator(symbol, precedence, invoke, semantics);
    }

    constexpr auto detail_with_postfix_entry(
        std::string_view symbol,
        int precedence,
        fw::static_function_ref<MathValue(const MathValue&)> invoke,
        CallableSemantics semantics = {}) const
    {
        return m_with_postfix_operator(symbol, precedence, invoke, semantics);
    }

    constexpr auto detail_with_function_entry(
        std::string_view name,
        fw::static_function_ref<MathValue(const MathValue*, std::size_t)> invoke,
        CallableSemantics semantics = {}) const
    {
        return m_with_function(name, invoke, semantics);
    }

    template <class Sig, auto Function>
    constexpr auto add_function(std::string_view name, CallableSemantics semantics = {}) const
    {
        return detail_add_function_nttp<Sig, Function>(name, semantics);
    }

    template <auto Function>
    constexpr auto add_function(std::string_view name, CallableSemantics semantics = {}) const
    {
        return add_function<fw::detail::fn_sig_t<decltype(Function)>, Function>(name, semantics);
    }

    template <class Sig, auto Function, std::size_t NameSize>
    constexpr auto add_function(const char (&name)[NameSize], CallableSemantics semantics = {}) const
    {
        return add_function<Sig, Function>(std::string_view(name, NameSize - 1), semantics);
    }

    template <auto Function, std::size_t NameSize>
    constexpr auto add_function(const char (&name)[NameSize], CallableSemantics semantics = {}) const
    {
        return add_function<Function>(std::string_view(name, NameSize - 1), semantics);
    }

    template <class Sig, auto Function>
    constexpr auto register_function(std::string_view name, CallableSemantics semantics = {}) const
    {
        return add_function<Sig, Function>(name, semantics);
    }

    template <auto Function>
    constexpr auto register_function(std::string_view name, CallableSemantics semantics = {}) const
    {
        return add_function<Function>(name, semantics);
    }

    template <class Sig, auto Function, std::size_t NameSize>
    constexpr auto register_function(const char (&name)[NameSize], CallableSemantics semantics = {}) const
    {
        return add_function<Sig, Function>(std::string_view(name, NameSize - 1), semantics);
    }

    template <auto Function, std::size_t NameSize>
    constexpr auto register_function(const char (&name)[NameSize], CallableSemantics semantics = {}) const
    {
        return add_function<Function>(std::string_view(name, NameSize - 1), semantics);
    }

    template <class Sig, class F>
    constexpr auto add_function(std::string_view name, F function, CallableSemantics semantics = {}) const
    {
        return m_with_typed_function<Sig>(name, +function, semantics);
    }

    constexpr auto add_function(std::string_view name, auto function, CallableSemantics semantics = {}) const
    {
        return m_with_typed_function<fw::detail::fn_sig_t<decltype(+function)>>(name, +function, semantics);
    }

    template <class Sig, class F>
    constexpr auto register_function(std::string_view name, F function, CallableSemantics semantics = {}) const
    {
        return m_with_typed_function<Sig>(name, +function, semantics);
    }

    template <class Sig, class F>
    constexpr auto add_prefix_operator(
        std::string_view symbol,
        F function,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {}) const
    {
        return m_with_typed_prefix_operator<Sig>(symbol, +function, precedence, semantics);
    }

    constexpr auto add_prefix_operator(
        std::string_view symbol,
        auto function,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {}) const
    {
        return m_with_typed_prefix_operator<fw::detail::fn_sig_t<decltype(+function)>>(symbol, +function, precedence, semantics);
    }

    template <class Sig, class F>
    constexpr auto register_prefix_operator(
        std::string_view symbol,
        F function,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {}) const
    {
        return m_with_typed_prefix_operator<Sig>(symbol, +function, precedence, semantics);
    }

    template <class Sig, auto Function>
    constexpr auto add_prefix_operator(
        std::string_view symbol,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {}) const
    {
        return detail_add_prefix_operator_nttp<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function>
    constexpr auto add_prefix_operator(
        std::string_view symbol,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {}) const
    {
        return add_prefix_operator<fw::detail::fn_sig_t<decltype(Function)>, Function>(symbol, precedence, semantics);
    }

    template <class Sig, auto Function>
    constexpr auto register_prefix_operator(
        std::string_view symbol,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {}) const
    {
        return add_prefix_operator<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function>
    constexpr auto register_prefix_operator(
        std::string_view symbol,
        int precedence = Precedence::Prefix,
        CallableSemantics semantics = {}) const
    {
        return add_prefix_operator<Function>(symbol, precedence, semantics);
    }

    template <class Sig, class F>
    constexpr auto add_postfix_operator(
        std::string_view symbol,
        F function,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {}) const
    {
        return m_with_typed_postfix_operator<Sig>(symbol, +function, precedence, semantics);
    }

    constexpr auto add_postfix_operator(
        std::string_view symbol,
        auto function,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {}) const
    {
        return m_with_typed_postfix_operator<fw::detail::fn_sig_t<decltype(+function)>>(symbol, +function, precedence, semantics);
    }

    template <class Sig, class F>
    constexpr auto register_postfix_operator(
        std::string_view symbol,
        F function,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {}) const
    {
        return m_with_typed_postfix_operator<Sig>(symbol, +function, precedence, semantics);
    }

    template <class Sig, auto Function>
    constexpr auto add_postfix_operator(
        std::string_view symbol,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {}) const
    {
        return detail_add_postfix_operator_nttp<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function>
    constexpr auto add_postfix_operator(
        std::string_view symbol,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {}) const
    {
        return add_postfix_operator<fw::detail::fn_sig_t<decltype(Function)>, Function>(symbol, precedence, semantics);
    }

    template <class Sig, auto Function>
    constexpr auto register_postfix_operator(
        std::string_view symbol,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {}) const
    {
        return add_postfix_operator<Sig, Function>(symbol, precedence, semantics);
    }

    template <auto Function>
    constexpr auto register_postfix_operator(
        std::string_view symbol,
        int precedence = Precedence::Postfix,
        CallableSemantics semantics = {}) const
    {
        return add_postfix_operator<Function>(symbol, precedence, semantics);
    }

    template <class Sig, class F>
    constexpr auto add_infix_operator(
        std::string_view symbol,
        F function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind = BinaryPolicyKind::None) const
    {
        return m_with_typed_infix_operator<Sig>(symbol, +function, precedence, associativity, semantics);
    }

    constexpr auto add_infix_operator(
        std::string_view symbol,
        auto function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None) const
    {
        return m_with_typed_infix_operator<fw::detail::fn_sig_t<decltype(+function)>>(
            symbol, +function, precedence, associativity, semantics);
    }

    template <class Sig, class F>
    constexpr auto register_infix_operator(
        std::string_view symbol,
        F function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None) const
    {
        return m_with_typed_infix_operator<Sig>(symbol, +function, precedence, associativity, semantics);
    }

    template <class Sig, auto Function>
    constexpr auto add_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None) const
    {
        return detail_add_infix_operator_nttp<Sig, Function>(symbol, precedence, associativity, semantics, binary_policy);
    }

    template <auto Function>
    constexpr auto add_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None) const
    {
        return add_infix_operator<fw::detail::fn_sig_t<decltype(Function)>, Function>(
            symbol, precedence, associativity, semantics, binary_policy);
    }

    template <class Sig, auto Function>
    constexpr auto register_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None) const
    {
        return add_infix_operator<Sig, Function>(symbol, precedence, associativity, semantics, binary_policy);
    }

    template <auto Function>
    constexpr auto register_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {},
        BinaryPolicyKind binary_policy = BinaryPolicyKind::None) const
    {
        return add_infix_operator<Function>(symbol, precedence, associativity, semantics, binary_policy);
    }

    constexpr std::optional<MathValue> find_value(std::string_view name) const
    {
        for (std::size_t index = 0; index < VariableCount; ++index)
        {
            if (m_data.m_values[index].name == name)
            {
                return m_data.m_values[index].value;
            }
        }
        return std::nullopt;
    }

    constexpr std::string_view match_infix_symbol(std::string_view text) const
    {
        std::string_view best;
        for (std::size_t index = 0; index < InfixCount; ++index)
        {
            const auto symbol = m_data.m_infix_operators[index].symbol;
            if (text.size() >= symbol.size() && text.substr(0, symbol.size()) == symbol && symbol.size() > best.size())
            {
                best = symbol;
            }
        }
        return best;
    }

    constexpr std::string_view match_prefix_symbol(std::string_view text) const
    {
        std::string_view best;
        for (std::size_t index = 0; index < PrefixCount; ++index)
        {
            const auto symbol = m_data.m_prefix_operators[index].symbol;
            if (text.size() >= symbol.size() && text.substr(0, symbol.size()) == symbol && symbol.size() > best.size())
            {
                best = symbol;
            }
        }
        return best;
    }

    constexpr std::string_view match_postfix_symbol(std::string_view text) const
    {
        std::string_view best;
        for (std::size_t index = 0; index < PostfixCount; ++index)
        {
            const auto symbol = m_data.m_postfix_operators[index].symbol;
            if (text.size() >= symbol.size() && text.substr(0, symbol.size()) == symbol && symbol.size() > best.size())
            {
                best = symbol;
            }
        }
        return best;
    }

    constexpr const ConstexprInfixOperatorEntry* find_infix_operator(std::string_view symbol) const
    {
        for (std::size_t index = 0; index < InfixCount; ++index)
        {
            if (m_data.m_infix_operators[index].symbol == symbol)
            {
                return &m_data.m_infix_operators[index];
            }
        }
        return nullptr;
    }

    constexpr const ConstexprPrefixOperatorEntry* find_prefix_operator(std::string_view symbol) const
    {
        for (std::size_t index = 0; index < PrefixCount; ++index)
        {
            if (m_data.m_prefix_operators[index].symbol == symbol)
            {
                return &m_data.m_prefix_operators[index];
            }
        }
        return nullptr;
    }

    constexpr const ConstexprPostfixOperatorEntry* find_postfix_operator(std::string_view symbol) const
    {
        for (std::size_t index = 0; index < PostfixCount; ++index)
        {
            if (m_data.m_postfix_operators[index].symbol == symbol)
            {
                return &m_data.m_postfix_operators[index];
            }
        }
        return nullptr;
    }

    constexpr const ConstexprFunctionEntry* find_function(std::string_view name) const
    {
        for (std::size_t index = 0; index < FunctionCount; ++index)
        {
            if (m_data.m_functions[index].name == name)
            {
                return &m_data.m_functions[index];
            }
        }
        return nullptr;
    }

private:
    template <class Sig, auto Function>
    static constexpr MathValue m_unary_invoke(const MathValue& argument)
    {
        using traits = detail::signature_traits<Sig>;
        static_assert(traits::arity == 1, "constexpr unary registration requires one argument");
        using result_t = typename traits::result_type;
        using argument_t = std::tuple_element_t<0, typename traits::args_tuple>;
        static_assert(is_supported_value_type_v<result_t>, "unsupported constexpr unary result type");
        static_assert(is_supported_value_type_v<argument_t>, "unsupported constexpr unary argument type");
        return MathValue(Function(argument.template cast<argument_t>()));
    }

    template <class Sig, auto Function>
    static constexpr MathValue m_binary_invoke(const MathValue& left, const MathValue& right)
    {
        using traits = detail::signature_traits<Sig>;
        static_assert(traits::arity == 2, "constexpr binary registration requires two arguments");
        using result_t = typename traits::result_type;
        using left_t = std::tuple_element_t<0, typename traits::args_tuple>;
        using right_t = std::tuple_element_t<1, typename traits::args_tuple>;
        static_assert(is_supported_value_type_v<result_t>, "unsupported constexpr binary result type");
        static_assert(is_supported_value_type_v<left_t>, "unsupported constexpr binary left type");
        static_assert(is_supported_value_type_v<right_t>, "unsupported constexpr binary right type");
        return MathValue(Function(left.template cast<left_t>(), right.template cast<right_t>()));
    }

    template <class Sig, auto Function>
    static constexpr MathValue m_function_invoke(const MathValue* arguments, std::size_t count)
    {
        using traits = detail::signature_traits<Sig>;
        if constexpr (traits::arity == 1)
        {
            if (count != 1)
            {
                throw std::invalid_argument("string_math: constexpr function expects one argument");
            }
            return m_unary_invoke<Sig, Function>(arguments[0]);
        }
        else if constexpr (traits::arity == 2)
        {
            if (count != 2)
            {
                throw std::invalid_argument("string_math: constexpr function expects two arguments");
            }
            return m_binary_invoke<Sig, Function>(arguments[0], arguments[1]);
        }
        else
        {
            static_assert(detail::always_false_v<Sig>, "constexpr functions support arity 1 or 2 only");
        }
    }

    template <class Sig, auto Function>
    constexpr auto detail_add_function_nttp(std::string_view name, CallableSemantics semantics) const
    {
        return m_with_function(
            name,
            fw::static_function_ref<MathValue(const MathValue*, std::size_t)>::template make<
                &StaticMathContext::template m_function_invoke<Sig, Function>>(),
            semantics);
    }

    template <class Sig, auto Function>
    constexpr auto detail_add_prefix_operator_nttp(
        std::string_view symbol,
        int precedence,
        CallableSemantics semantics) const
    {
        return m_with_prefix_operator(
            symbol,
            precedence,
            fw::static_function_ref<MathValue(const MathValue&)>::template make<
                &StaticMathContext::template m_unary_invoke<Sig, Function>>(),
            semantics);
    }

    template <class Sig, auto Function>
    constexpr auto detail_add_postfix_operator_nttp(
        std::string_view symbol,
        int precedence,
        CallableSemantics semantics) const
    {
        return m_with_postfix_operator(
            symbol,
            precedence,
            fw::static_function_ref<MathValue(const MathValue&)>::template make<
                &StaticMathContext::template m_unary_invoke<Sig, Function>>(),
            semantics);
    }

    template <class Sig, auto Function>
    constexpr auto detail_add_infix_operator_nttp(
        std::string_view symbol,
        int precedence,
        Associativity associativity,
        CallableSemantics semantics,
        BinaryPolicyKind = BinaryPolicyKind::None) const
    {
        return m_with_infix_operator(
            symbol,
            precedence,
            associativity,
            fw::static_function_ref<MathValue(const MathValue&, const MathValue&)>::template make<
                &StaticMathContext::template m_binary_invoke<Sig, Function>>(),
            semantics);
    }

    template <class Sig>
    static constexpr void m_validate_same_type_unary_signature()
    {
        using traits = detail::signature_traits<Sig>;
        static_assert(traits::arity == 1, "constexpr unary registration requires one argument");
        using result_t = typename traits::result_type;
        using argument_t = std::tuple_element_t<0, typename traits::args_tuple>;
        static_assert(is_supported_value_type_v<result_t>, "unsupported constexpr unary result type");
        static_assert(is_supported_value_type_v<argument_t>, "unsupported constexpr unary argument type");
        static_assert(std::is_same_v<result_t, argument_t>, "constexpr function-parameter registration currently requires same-type unary signatures");
    }

    template <class Sig>
    static constexpr void m_validate_same_type_binary_signature()
    {
        using traits = detail::signature_traits<Sig>;
        static_assert(traits::arity == 2, "constexpr binary registration requires two arguments");
        using result_t = typename traits::result_type;
        using left_t = std::tuple_element_t<0, typename traits::args_tuple>;
        using right_t = std::tuple_element_t<1, typename traits::args_tuple>;
        static_assert(is_supported_value_type_v<result_t>, "unsupported constexpr binary result type");
        static_assert(is_supported_value_type_v<left_t>, "unsupported constexpr binary left type");
        static_assert(is_supported_value_type_v<right_t>, "unsupported constexpr binary right type");
        static_assert(
            std::is_same_v<result_t, left_t> && std::is_same_v<left_t, right_t>,
            "constexpr function-parameter registration currently requires same-type binary signatures");
    }

    template <class Sig, class F>
    static constexpr auto m_make_unary_typed_callable(F function)
    {
        m_validate_same_type_unary_signature<Sig>();
        fw::static_function<STRING_MATH_ALL_UNARY_SAME_TYPE_SIGNATURES> callable;
        callable.template bind<Sig>(+function);
        return callable;
    }

    template <class Sig, class F>
    static constexpr auto m_make_binary_typed_callable(F function)
    {
        m_validate_same_type_binary_signature<Sig>();
        fw::static_function<STRING_MATH_ALL_BINARY_SAME_TYPE_SIGNATURES> callable;
        callable.template bind<Sig>(+function);
        return callable;
    }

    template <class Sig, class F>
    constexpr auto m_with_typed_function(std::string_view name, F function, CallableSemantics semantics) const
    {
        using traits = detail::signature_traits<Sig>;
        if constexpr (traits::arity == 1)
        {
            using argument_t = std::tuple_element_t<0, typename traits::args_tuple>;
            StaticMathContext<VariableCount, InfixCount, FunctionCount + 1, PrefixCount, PostfixCount> next;
            m_copy_into(next);
            next.m_data.m_functions[FunctionCount] = ConstexprFunctionEntry{
                name,
                {},
                semantics,
                1,
                true,
                value_type_of<argument_t>::value,
                ValueType::Int,
                m_make_unary_typed_callable<Sig>(function),
                {}};
            return next;
        }
        else if constexpr (traits::arity == 2)
        {
            using left_t = std::tuple_element_t<0, typename traits::args_tuple>;
            using right_t = std::tuple_element_t<1, typename traits::args_tuple>;
            StaticMathContext<VariableCount, InfixCount, FunctionCount + 1, PrefixCount, PostfixCount> next;
            m_copy_into(next);
            next.m_data.m_functions[FunctionCount] = ConstexprFunctionEntry{
                name,
                {},
                semantics,
                2,
                true,
                value_type_of<left_t>::value,
                value_type_of<right_t>::value,
                {},
                m_make_binary_typed_callable<Sig>(function)};
            return next;
        }
        else
        {
            static_assert(detail::always_false_v<Sig>, "constexpr functions support arity 1 or 2 only");
        }
    }

    template <class Sig, class F>
    constexpr auto m_with_typed_prefix_operator(
        std::string_view symbol,
        F function,
        int precedence,
        CallableSemantics semantics) const
    {
        using argument_t = std::tuple_element_t<0, typename detail::signature_traits<Sig>::args_tuple>;
        StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount + 1, PostfixCount> next;
        m_copy_into(next);
        next.m_data.m_prefix_operators[PrefixCount] = ConstexprPrefixOperatorEntry{
            symbol,
            precedence,
            {},
            semantics,
            true,
            value_type_of<argument_t>::value,
            m_make_unary_typed_callable<Sig>(function)};
        return next;
    }

    template <class Sig, class F>
    constexpr auto m_with_typed_postfix_operator(
        std::string_view symbol,
        F function,
        int precedence,
        CallableSemantics semantics) const
    {
        using argument_t = std::tuple_element_t<0, typename detail::signature_traits<Sig>::args_tuple>;
        StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount + 1> next;
        m_copy_into(next);
        next.m_data.m_postfix_operators[PostfixCount] = ConstexprPostfixOperatorEntry{
            symbol,
            precedence,
            {},
            semantics,
            true,
            value_type_of<argument_t>::value,
            m_make_unary_typed_callable<Sig>(function)};
        return next;
    }

    template <class Sig, class F>
    constexpr auto m_with_typed_infix_operator(
        std::string_view symbol,
        F function,
        int precedence,
        Associativity associativity,
        CallableSemantics semantics) const
    {
        using traits = detail::signature_traits<Sig>;
        using left_t = std::tuple_element_t<0, typename traits::args_tuple>;
        using right_t = std::tuple_element_t<1, typename traits::args_tuple>;
        StaticMathContext<VariableCount, InfixCount + 1, FunctionCount, PrefixCount, PostfixCount> next;
        m_copy_into(next);
        next.m_data.m_infix_operators[InfixCount] = ConstexprInfixOperatorEntry{
            symbol,
            precedence,
            associativity,
            {},
            semantics,
            true,
            value_type_of<left_t>::value,
            value_type_of<right_t>::value,
            m_make_binary_typed_callable<Sig>(function)};
        return next;
    }

    template <class NextContext>
    constexpr void m_copy_into(NextContext& next) const
    {
        for (std::size_t index = 0; index < VariableCount; ++index)
        {
            next.m_data.m_values[index] = m_data.m_values[index];
        }
        for (std::size_t index = 0; index < InfixCount; ++index)
        {
            next.m_data.m_infix_operators[index] = m_data.m_infix_operators[index];
        }
        for (std::size_t index = 0; index < FunctionCount; ++index)
        {
            next.m_data.m_functions[index] = m_data.m_functions[index];
        }
        for (std::size_t index = 0; index < PrefixCount; ++index)
        {
            next.m_data.m_prefix_operators[index] = m_data.m_prefix_operators[index];
        }
        for (std::size_t index = 0; index < PostfixCount; ++index)
        {
            next.m_data.m_postfix_operators[index] = m_data.m_postfix_operators[index];
        }
    }

    constexpr auto detail_set_value(std::string_view name, MathValue value) const
    {
        StaticMathContext<VariableCount + 1, InfixCount, FunctionCount, PrefixCount, PostfixCount> next;
        m_copy_into(next);
        next.m_data.m_values[VariableCount] = ConstexprVariableEntry{name, value};
        return next;
    }

    constexpr auto detail_with_value(std::string_view name, MathValue value) const
    {
        return detail_set_value(name, value);
    }

    constexpr auto m_with_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity,
        fw::static_function_ref<MathValue(const MathValue&, const MathValue&)> invoke,
        CallableSemantics semantics) const
    {
        StaticMathContext<VariableCount, InfixCount + 1, FunctionCount, PrefixCount, PostfixCount> next;
        m_copy_into(next);
        next.m_data.m_infix_operators[InfixCount] =
            ConstexprInfixOperatorEntry{symbol, precedence, associativity, invoke, semantics};
        return next;
    }

    constexpr auto m_with_function(
        std::string_view name,
        fw::static_function_ref<MathValue(const MathValue*, std::size_t)> invoke,
        CallableSemantics semantics) const
    {
        StaticMathContext<VariableCount, InfixCount, FunctionCount + 1, PrefixCount, PostfixCount> next;
        m_copy_into(next);
        next.m_data.m_functions[FunctionCount] = ConstexprFunctionEntry{name, invoke, semantics};
        return next;
    }

    constexpr auto m_with_prefix_operator(
        std::string_view symbol,
        int precedence,
        fw::static_function_ref<MathValue(const MathValue&)> invoke,
        CallableSemantics semantics) const
    {
        StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount + 1, PostfixCount> next;
        m_copy_into(next);
        next.m_data.m_prefix_operators[PrefixCount] = ConstexprPrefixOperatorEntry{symbol, precedence, invoke, semantics};
        return next;
    }

    constexpr auto m_with_postfix_operator(
        std::string_view symbol,
        int precedence,
        fw::static_function_ref<MathValue(const MathValue&)> invoke,
        CallableSemantics semantics) const
    {
        StaticMathContext<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount + 1> next;
        m_copy_into(next);
        next.m_data.m_postfix_operators[PostfixCount] =
            ConstexprPostfixOperatorEntry{symbol, precedence, invoke, semantics};
        return next;
    }

    StaticContextData<VariableCount, InfixCount, FunctionCount, PrefixCount, PostfixCount> m_data{};

    template <std::size_t, std::size_t, std::size_t, std::size_t, std::size_t>
    friend class StaticMathContext;
};

} // namespace detail

constexpr auto MathContext::compile_time()
{
    return detail::StaticMathContext<>{};
}
#endif

} // namespace string_math
