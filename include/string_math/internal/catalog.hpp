#pragma once

#include <cstddef>
#include <limits>
#include <span>
#include <string_view>
#include <vector>

#include <string_math/diagnostics/result.hpp>
#include <string_math/semantics/semantics.hpp>
#include <string_math/value/policy.hpp>
#include <string_math/value/types.hpp>
#include <string_math/value/value.hpp>

namespace string_math::internal
{

enum class CallableKind
{
    Function,
    PrefixOperator,
    PostfixOperator,
    InfixOperator
};

enum class ArityKind
{
    Fixed,
    Variadic
};

inline constexpr std::size_t k_unbounded_arity = std::numeric_limits<std::size_t>::max();

using callable_invoke_fn = MathValue (*)(const void*, const MathValue*, std::size_t);
using callable_policy_fn =
    Result<MathValue> (*)(const void*, const MathValue*, std::size_t, const EvaluationPolicy&, std::string_view);

struct ValueDescriptorView
{
    std::string_view name;
    MathValue value;
};

struct CallableOverloadView
{
    ValueType result_type{};
    ArityKind arity_kind{ArityKind::Fixed};
    std::size_t min_arity{0};
    std::size_t max_arity{0};
    std::span<const ValueType> arg_types{};
    CallableSemantics semantics{};
    const void* state{nullptr};
    callable_invoke_fn invoke{nullptr};
    const void* policy_state{nullptr};
    callable_policy_fn policy_invoke{nullptr};

    constexpr bool accepts_arity(std::size_t arity) const noexcept
    {
        return arity >= min_arity && (max_arity == k_unbounded_arity || arity <= max_arity);
    }
};

struct CallableDescriptorView
{
    std::string_view name_or_symbol;
    CallableKind kind{CallableKind::Function};
    int precedence{0};
    Associativity associativity{Associativity::Left};
    std::span<const CallableOverloadView> overloads{};
};

struct CollectedCallable
{
    int precedence{0};
    Associativity associativity{Associativity::Left};
    std::vector<CallableOverloadView> overloads;

    constexpr explicit operator bool() const noexcept
    {
        return !overloads.empty();
    }
};

} // namespace string_math::internal
