#pragma once
// Do not include this file directly. Use <string_math/string_math.hpp> or individual public headers.

#include <array>

#include <string_math/internal/config.hpp>
#include <string_math/internal/type_conversion.hpp>
#include <string_math/value/value.hpp>
#include <string_math/semantics/semantics.hpp>

namespace string_math::internal
{

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION

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

#endif // STRING_MATH_HAS_CONSTEXPR_EVALUATION

} // namespace string_math::internal
