#pragma once

#include <array>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <string_math/callable.hpp>
#include <string_math/internal/config.hpp>
#include <string_math/internal/overload_impl.hpp>

namespace string_math::internal
{

struct LiteralParserEntry
{
    std::string prefix;
    std::string suffix;
    LiteralParser parser;
};

struct CallableEntry
{
    int precedence{0};
    Associativity associativity{Associativity::Left};
    std::vector<RuntimeCallableOverload> overloads;

    CollectedCallable collect() const
    {
        CollectedCallable collected;
        collected.precedence = precedence;
        collected.associativity = associativity;
        collected.overloads.reserve(overloads.size());
        for (const auto& overload : overloads)
        {
            collected.overloads.push_back(overload.view());
        }
        return collected;
    }
};

using PrefixOperatorEntry = CallableEntry;
using PostfixOperatorEntry = CallableEntry;
using InfixOperatorEntry = CallableEntry;
using FunctionEntry = CallableEntry;

struct RuntimeContextData
{
    std::shared_ptr<RuntimeContextData> m_parent;
    std::unordered_map<std::string, MathValue> m_values;
    std::unordered_map<std::string, FunctionEntry> m_functions;
    std::unordered_map<std::string, PrefixOperatorEntry> m_prefix_operators;
    std::unordered_map<std::string, PostfixOperatorEntry> m_postfix_operators;
    std::unordered_map<std::string, InfixOperatorEntry> m_infix_operators;
    std::vector<LiteralParserEntry> m_literal_parsers;
    EvaluationPolicy m_policy{};
};

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION

struct StaticValueEntry
{
    std::string_view name;
    MathValue value;
};

struct StaticCallableEntry
{
    std::string_view name_or_symbol;
    int precedence{0};
    Associativity associativity{Associativity::Left};
    CallableOverloadView overload{};
};

constexpr MathValue invoke_constexpr_overload(
    const CallableOverloadView& overload,
    const MathValue* arguments,
    std::size_t count,
    const EvaluationPolicy& policy,
    std::string_view token)
{
    const MathArgsView args_view(arguments, count);
    if (overload.policy_invoke)
    {
        const auto result = overload.policy_invoke(args_view, policy, token);
        if (!result)
        {
            throw std::invalid_argument(result.error().message());
        }
        return result.value();
    }
    if (overload.raw_policy_invoke)
    {
        const auto result = overload.raw_policy_invoke(arguments, count, policy, token);
        if (!result)
        {
            throw std::invalid_argument(result.error().message());
        }
        return result.value();
    }
    if (overload.raw_invoke)
    {
        return overload.raw_invoke(arguments, count);
    }
    return overload.invoke(args_view);
}

template <
    std::size_t VariableCapacity,
    std::size_t InfixCapacity,
    std::size_t FunctionCapacity,
    std::size_t PrefixCapacity,
    std::size_t PostfixCapacity>
struct StaticContextData
{
    std::array<StaticValueEntry, VariableCapacity> m_values{};
    std::size_t m_value_count{0};
    std::array<StaticCallableEntry, InfixCapacity> m_infix_operators{};
    std::size_t m_infix_operator_count{0};
    std::array<StaticCallableEntry, FunctionCapacity> m_functions{};
    std::size_t m_function_count{0};
    std::array<StaticCallableEntry, PrefixCapacity> m_prefix_operators{};
    std::size_t m_prefix_operator_count{0};
    std::array<StaticCallableEntry, PostfixCapacity> m_postfix_operators{};
    std::size_t m_postfix_operator_count{0};
    EvaluationPolicy m_policy{};
};

#endif // STRING_MATH_HAS_CONSTEXPR_EVALUATION

} // namespace string_math::internal
