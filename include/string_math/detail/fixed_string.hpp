#pragma once

#include <array>
#include <string_view>

namespace string_math::detail
{

template <std::size_t N>
struct fixed_string
{
    std::array<char, N> value{};

    constexpr fixed_string(const char (&text)[N])
    {
        for (std::size_t index = 0; index < N; ++index)
        {
            value[index] = text[index];
        }
    }

    constexpr std::string_view view() const noexcept
    {
        return std::string_view(value.data(), N - 1);
    }
};

template <std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

} // namespace string_math::detail
