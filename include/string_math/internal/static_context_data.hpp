#pragma once

#include <array>
#include <stdexcept>

#include <string_math/internal/catalog.hpp>
#include <string_math/internal/config.hpp>

namespace string_math::internal
{

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
    if (overload.policy_invoke != nullptr)
    {
        const auto result = overload.policy_invoke(overload.policy_state, arguments, count, policy, token);
        if (!result)
        {
            throw std::invalid_argument(result.error().message());
        }
        return result.value();
    }
    return overload.invoke(overload.state, arguments, count);
}

template <
    std::size_t VariableCount,
    std::size_t InfixCount,
    std::size_t FunctionCount,
    std::size_t PrefixCount,
    std::size_t PostfixCount>
struct StaticContextData
{
    std::array<StaticValueEntry, VariableCount> m_values{};
    std::array<StaticCallableEntry, InfixCount> m_infix_operators{};
    std::array<StaticCallableEntry, FunctionCount> m_functions{};
    std::array<StaticCallableEntry, PrefixCount> m_prefix_operators{};
    std::array<StaticCallableEntry, PostfixCount> m_postfix_operators{};
    EvaluationPolicy m_policy{};
};

#endif // STRING_MATH_HAS_CONSTEXPR_EVALUATION

} // namespace string_math::internal
