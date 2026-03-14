#pragma once

#include <cstdint>

namespace string_math
{

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

} // namespace string_math
