#pragma once
// Do not include this file directly. Use <string_math/string_math.hpp> or individual public headers.

#include <array>
#include <cstddef>
#include <span>
#include <stdexcept>

namespace string_math::internal
{

// ---------------------------------------------------------------------------
// FixedVector<T, N>
//
// A fixed-capacity vector that is fully constexpr-capable without relying on
// transient heap allocation. Safe to use inside consteval functions.
//
// T must be default-constructible (the backing array is always fully initialized).
// ---------------------------------------------------------------------------

/// Maximum overloads per symbol/function name in the constexpr context.
inline constexpr std::size_t k_max_overloads_per_symbol = 16;

/// Maximum function arguments supported in the constexpr evaluation path.
inline constexpr std::size_t k_max_constexpr_args = 32;

template <class T, std::size_t N>
class FixedVector
{
public:
    using value_type = T;
    using size_type = std::size_t;
    using iterator = T*;
    using const_iterator = const T*;

    constexpr FixedVector() = default;

    [[nodiscard]] constexpr bool empty() const noexcept { return m_size == 0; }
    [[nodiscard]] constexpr size_type size() const noexcept { return m_size; }
    [[nodiscard]] static constexpr size_type capacity() noexcept { return N; }

    constexpr void push_back(const T& value)
    {
        if (m_size >= N)
        {
            throw std::out_of_range("string_math: FixedVector capacity exceeded");
        }
        m_data[m_size++] = value;
    }

    constexpr T& operator[](size_type index) noexcept { return m_data[index]; }
    constexpr const T& operator[](size_type index) const noexcept { return m_data[index]; }

    constexpr T* data() noexcept { return m_data.data(); }
    constexpr const T* data() const noexcept { return m_data.data(); }

    constexpr iterator begin() noexcept { return m_data.data(); }
    constexpr iterator end() noexcept { return m_data.data() + m_size; }
    constexpr const_iterator begin() const noexcept { return m_data.data(); }
    constexpr const_iterator end() const noexcept { return m_data.data() + m_size; }

    [[nodiscard]] constexpr std::span<const T> as_span() const noexcept
    {
        return std::span<const T>(m_data.data(), m_size);
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept { return m_size > 0; }

private:
    std::array<T, N> m_data{};
    size_type m_size{0};
};

} // namespace string_math::internal
