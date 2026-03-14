#pragma once
// Do not include this file directly. Use <string_math/string_math.hpp> or individual public headers.

#include <optional>
#include <string_view>

#include <string_math/context/context.hpp>
#include <string_math/internal/static_context_data.hpp>

namespace string_math::internal
{

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION

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
        using traits = signature_traits<Sig>;
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
        using traits = signature_traits<Sig>;
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
        using traits = signature_traits<Sig>;
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
            static_assert(always_false_v<Sig>, "constexpr functions support arity 1 or 2 only");
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
        using traits = signature_traits<Sig>;
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
        using traits = signature_traits<Sig>;
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
        using traits = signature_traits<Sig>;
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
                value_type_of<argument_t>::value,
                m_make_unary_typed_callable<Sig>(function)};
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
            static_assert(always_false_v<Sig>, "constexpr functions support arity 1 or 2 only");
        }
    }

    template <class Sig, class F>
    constexpr auto m_with_typed_prefix_operator(
        std::string_view symbol,
        F function,
        int precedence,
        CallableSemantics semantics) const
    {
        using argument_t = std::tuple_element_t<0, typename signature_traits<Sig>::args_tuple>;
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
        using argument_t = std::tuple_element_t<0, typename signature_traits<Sig>::args_tuple>;
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
        using traits = signature_traits<Sig>;
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

#endif // STRING_MATH_HAS_CONSTEXPR_EVALUATION

} // namespace string_math::internal

namespace string_math
{

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
// Definition of MathContext::compile_time() declared in context/context.hpp
inline constexpr auto MathContext::compile_time()
{
    return internal::StaticMathContext<>{};
}
#endif

} // namespace string_math
