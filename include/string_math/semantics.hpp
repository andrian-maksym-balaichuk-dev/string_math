#pragma once

#include <cstdint>

namespace string_math
{

enum class ArithmeticKind
{
    None,
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Power,
    Log,
    Min,
    Max,
    CompareEqual,
    CompareNotEqual,
    CompareLess,
    CompareLessEqual,
    CompareGreater,
    CompareGreaterEqual,
    LogicalAnd,
    LogicalOr,
    LogicalNot,
    Negate,
    Identity,
    Factorial
};

enum class BinaryPolicyKind
{
    None,
    IntegralAddOverflow,
    IntegralSubtractOverflow,
    IntegralMultiplyOverflow
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
    ArithmeticKind arithmetic_kind{ArithmeticKind::None};
    BinaryPolicyKind binary_policy{BinaryPolicyKind::None};
    std::uint32_t flags{static_cast<std::uint32_t>(CallableFlag::None)};

    constexpr CallableSemantics() = default;

    constexpr CallableSemantics(ArithmeticKind kind, std::uint32_t flag_bits) noexcept
        : arithmetic_kind(kind), flags(flag_bits)
    {}

    constexpr CallableSemantics(ArithmeticKind kind, BinaryPolicyKind policy_kind, std::uint32_t flag_bits) noexcept
        : arithmetic_kind(kind), binary_policy(policy_kind), flags(flag_bits)
    {}

    constexpr bool has_flag(CallableFlag flag) const noexcept
    {
        return (flags & static_cast<std::uint32_t>(flag)) != 0;
    }

    constexpr bool empty() const noexcept
    {
        return arithmetic_kind == ArithmeticKind::None && binary_policy == BinaryPolicyKind::None &&
            flags == static_cast<std::uint32_t>(CallableFlag::None);
    }

    constexpr CallableSemantics with_binary_policy(BinaryPolicyKind policy_kind) const noexcept
    {
        CallableSemantics copy = *this;
        copy.binary_policy = policy_kind;
        if (policy_kind != BinaryPolicyKind::None)
        {
            copy.flags |= static_cast<std::uint32_t>(CallableFlag::OverflowSensitive);
        }
        return copy;
    }
};

struct OperatorSemantics : CallableSemantics
{
    constexpr OperatorSemantics() = default;

    constexpr OperatorSemantics(ArithmeticKind kind, std::uint32_t flag_bits) noexcept
        : CallableSemantics(kind, flag_bits)
    {}

    constexpr OperatorSemantics(ArithmeticKind kind, BinaryPolicyKind policy_kind, std::uint32_t flag_bits) noexcept
        : CallableSemantics(kind, policy_kind, flag_bits)
    {}

    constexpr OperatorSemantics with_binary_policy(BinaryPolicyKind policy_kind) const noexcept
    {
        return {arithmetic_kind, policy_kind, flags};
    }

    static constexpr OperatorSemantics arithmetic_add()
    {
        return {ArithmeticKind::Add,
                BinaryPolicyKind::IntegralAddOverflow,
                CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::OverflowSensitive |
                    CallableFlag::Foldable};
    }

    static constexpr OperatorSemantics arithmetic_subtract()
    {
        return {ArithmeticKind::Subtract,
                BinaryPolicyKind::IntegralSubtractOverflow,
                CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::OverflowSensitive |
                    CallableFlag::Foldable};
    }

    static constexpr OperatorSemantics arithmetic_multiply()
    {
        return {ArithmeticKind::Multiply,
                BinaryPolicyKind::IntegralMultiplyOverflow,
                CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::OverflowSensitive |
                    CallableFlag::Foldable};
    }

    static constexpr OperatorSemantics arithmetic_divide()
    {
        return {ArithmeticKind::Divide, CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::Foldable};
    }

    static constexpr OperatorSemantics arithmetic_modulo()
    {
        return {ArithmeticKind::Modulo, CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::Foldable};
    }

    static constexpr OperatorSemantics power()
    {
        return {ArithmeticKind::Power, CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::Foldable};
    }

    static constexpr OperatorSemantics logarithm()
    {
        return {ArithmeticKind::Log, CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::Foldable};
    }

    static constexpr OperatorSemantics min_op()
    {
        return {ArithmeticKind::Min, CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::Foldable};
    }

    static constexpr OperatorSemantics max_op()
    {
        return {ArithmeticKind::Max, CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::Foldable};
    }

    static constexpr OperatorSemantics comparison(ArithmeticKind kind)
    {
        return {kind, CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::Foldable};
    }

    static constexpr OperatorSemantics logical_and_op()
    {
        return {ArithmeticKind::LogicalAnd,
                CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::Foldable |
                    CallableFlag::ShortCircuitCandidate};
    }

    static constexpr OperatorSemantics logical_or_op()
    {
        return {ArithmeticKind::LogicalOr,
                CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::Foldable |
                    CallableFlag::ShortCircuitCandidate};
    }

    static constexpr OperatorSemantics logical_not_op()
    {
        return {ArithmeticKind::LogicalNot, CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::Foldable};
    }

    static constexpr OperatorSemantics negate()
    {
        return {ArithmeticKind::Negate, CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::Foldable};
    }

    static constexpr OperatorSemantics identity()
    {
        return {ArithmeticKind::Identity, CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::Foldable};
    }

    static constexpr OperatorSemantics factorial()
    {
        return {ArithmeticKind::Factorial, CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::Foldable};
    }
};

struct FunctionSemantics : CallableSemantics
{
    constexpr FunctionSemantics() = default;

    constexpr FunctionSemantics(ArithmeticKind kind, std::uint32_t flag_bits) noexcept
        : CallableSemantics(kind, flag_bits)
    {}

    static constexpr FunctionSemantics pure()
    {
        return {ArithmeticKind::None, CallableFlag::Pure | CallableFlag::Deterministic | CallableFlag::Foldable};
    }
};

} // namespace string_math
