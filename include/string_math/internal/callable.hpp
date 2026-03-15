#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fw/function_ref.hpp>
#include <fw/function_wrapper.hpp>
#include <fw/move_only_function_wrapper.hpp>

#include <string_math/diagnostics/result.hpp>
#include <string_math/value/policy.hpp>
#include <string_math/value/value.hpp>

namespace string_math
{

enum class ArityKind
{
    Fixed,
    Variadic
};

enum class CallableFlag : std::uint32_t
{
    None = 0,
    Pure = 1u << 0u,
    Deterministic = 1u << 1u,
    OverflowSensitive = 1u << 2u,
    IntegralOnly = 1u << 3u,
    FloatingOnly = 1u << 4u,
    ShortCircuitCandidate = 1u << 5u,
    Foldable = 1u << 6u
};

constexpr std::uint32_t operator|(CallableFlag left, CallableFlag right) noexcept
{
    return static_cast<std::uint32_t>(left) | static_cast<std::uint32_t>(right);
}

constexpr std::uint32_t operator|(std::uint32_t left, CallableFlag right) noexcept
{
    return left | static_cast<std::uint32_t>(right);
}

struct CallableSemantics
{
    std::uint32_t flags{static_cast<std::uint32_t>(CallableFlag::None)};

    constexpr CallableSemantics() = default;

    constexpr explicit CallableSemantics(std::uint32_t flag_bits) noexcept
        : flags(flag_bits)
    {}

    constexpr bool has_flag(CallableFlag flag) const noexcept
    {
        return (flags & static_cast<std::uint32_t>(flag)) != 0;
    }

    constexpr bool empty() const noexcept
    {
        return flags == static_cast<std::uint32_t>(CallableFlag::None);
    }
};

using MathArgPointer = const MathValue*;
using MathArgCount = std::size_t;

struct MathArgsView
{
    static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

    constexpr MathArgsView() = default;

    constexpr MathArgsView(MathArgPointer data, MathArgCount size) noexcept
        : m_data(data),
          m_size(size)
    {}

    [[nodiscard]] constexpr MathArgPointer data() const noexcept
    {
        return m_data;
    }

    [[nodiscard]] constexpr MathArgCount size() const noexcept
    {
        return m_size;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return m_size == 0;
    }

    [[nodiscard]] constexpr const MathValue& operator[](MathArgCount index) const
    {
        if (index >= m_size)
        {
            throw std::out_of_range("string_math: MathArgsView index out of range");
        }
        return m_data[index];
    }

    [[nodiscard]] constexpr MathArgPointer begin() const noexcept
    {
        return m_data;
    }

    [[nodiscard]] constexpr MathArgPointer end() const noexcept
    {
        return m_data + m_size;
    }

    [[nodiscard]] constexpr MathArgsView subspan(MathArgCount offset, MathArgCount count = npos) const
    {
        if (offset > m_size)
        {
            throw std::out_of_range("string_math: MathArgsView subspan offset out of range");
        }

        const MathArgCount remaining = m_size - offset;
        const MathArgCount span_size = count == npos ? remaining : count;
        if (span_size > remaining)
        {
            throw std::out_of_range("string_math: MathArgsView subspan count out of range");
        }

        return MathArgsView(m_data + offset, span_size);
    }

private:
    MathArgPointer m_data{nullptr};
    MathArgCount m_size{0};
};

using MathInvokeSignature = MathValue(MathArgsView);
using RawMathInvokeSignature = MathValue(MathArgPointer, MathArgCount);
using MathPolicySignature = Result<MathValue>(MathArgsView, const EvaluationPolicy&, std::string_view);
using RawMathPolicySignature =
    Result<MathValue>(MathArgPointer, MathArgCount, const EvaluationPolicy&, std::string_view);

using MathCallableView = fw::function_ref<MathInvokeSignature>;
using MathPolicyCallableView = fw::function_ref<MathPolicySignature>;
using RawMathCallableView = fw::function_ref<RawMathInvokeSignature>;
using RawMathPolicyCallableView = fw::function_ref<RawMathPolicySignature>;

using MathCallable = fw::function_wrapper<MathInvokeSignature>;
using MathPolicyCallable = fw::function_wrapper<MathPolicySignature>;
using RawMathCallable = fw::function_wrapper<RawMathInvokeSignature>;
using RawMathPolicyCallable = fw::function_wrapper<RawMathPolicySignature>;

using MoveOnlyMathCallable = fw::move_only_function_wrapper<MathInvokeSignature>;
using MoveOnlyMathPolicyCallable = fw::move_only_function_wrapper<MathPolicySignature>;
using MoveOnlyRawMathCallable = fw::move_only_function_wrapper<RawMathInvokeSignature>;
using MoveOnlyRawMathPolicyCallable = fw::move_only_function_wrapper<RawMathPolicySignature>;

using LiteralParser = fw::function_wrapper<std::optional<MathValue>(std::string_view)>;
using MoveOnlyLiteralParser = fw::move_only_function_wrapper<std::optional<MathValue>(std::string_view)>;

struct CallableOverloadInfo
{
    ValueType result_type{};
    ArityKind arity_kind{ArityKind::Fixed};
    std::size_t min_arity{0};
    std::size_t max_arity{0};
    std::vector<ValueType> argument_types;
    CallableSemantics semantics{};
};

struct FunctionInfo
{
    std::string name;
    std::vector<CallableOverloadInfo> overloads;
};

struct OperatorInfo
{
    std::string symbol;
    int precedence{0};
    Associativity associativity{Associativity::Left};
    std::vector<CallableOverloadInfo> overloads;
};

struct LiteralParserInfo
{
    std::string prefix;
    std::string suffix;
};

constexpr MathCallable make_math_callable(MathCallable callable)
{
    return callable;
}

constexpr MathCallable make_math_callable(RawMathCallable callable)
{
    return MathCallable([callable = std::move(callable)](MathArgsView arguments) mutable {
        return callable(arguments.data(), arguments.size());
    });
}

constexpr MoveOnlyMathCallable make_move_only_math_callable(MoveOnlyMathCallable callable)
{
    return callable;
}

constexpr MoveOnlyMathCallable make_move_only_math_callable(MoveOnlyRawMathCallable callable)
{
    return MoveOnlyMathCallable([callable = std::move(callable)](MathArgsView arguments) mutable {
        return callable(arguments.data(), arguments.size());
    });
}

constexpr MathPolicyCallable make_math_policy_callable(MathPolicyCallable callable)
{
    return callable;
}

constexpr MathPolicyCallable make_math_policy_callable(RawMathPolicyCallable callable)
{
    return MathPolicyCallable([callable = std::move(callable)](
                                  MathArgsView arguments,
                                  const EvaluationPolicy& policy,
                                  std::string_view token) mutable {
        return callable(arguments.data(), arguments.size(), policy, token);
    });
}

constexpr MoveOnlyMathPolicyCallable make_move_only_math_policy_callable(MoveOnlyMathPolicyCallable callable)
{
    return callable;
}

constexpr MoveOnlyMathPolicyCallable make_move_only_math_policy_callable(MoveOnlyRawMathPolicyCallable callable)
{
    return MoveOnlyMathPolicyCallable(
        [callable = std::move(callable)](MathArgsView arguments, const EvaluationPolicy& policy, std::string_view token) mutable {
            return callable(arguments.data(), arguments.size(), policy, token);
        });
}

} // namespace string_math

namespace string_math::internal
{

enum class CallableKind
{
    Function,
    PrefixOperator,
    PostfixOperator,
    InfixOperator
};

inline constexpr std::size_t k_unbounded_arity = std::numeric_limits<std::size_t>::max();

using ArityKind = ::string_math::ArityKind;
using callable_invoke_view = MathCallableView;
using callable_policy_view = MathPolicyCallableView;
using callable_invoke_wrapper = MathCallable;
using callable_policy_wrapper = MathPolicyCallable;
using raw_callable_invoke_wrapper = RawMathCallable;
using raw_callable_policy_wrapper = RawMathPolicyCallable;
using move_only_callable_invoke_wrapper = MoveOnlyMathCallable;
using move_only_callable_policy_wrapper = MoveOnlyMathPolicyCallable;
using callable_invoke_fn = MathCallableView;
using callable_policy_fn = MathPolicyCallableView;

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
    callable_invoke_wrapper invoke{};
    raw_callable_invoke_wrapper raw_invoke{};
    callable_policy_wrapper policy_invoke{};
    raw_callable_policy_wrapper raw_policy_invoke{};

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
