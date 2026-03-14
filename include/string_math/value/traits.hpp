#pragma once

#include <string_math/internal/type_conversion.hpp>

namespace string_math
{

template <class T, class Enable = void>
struct ValueTraits
{
    static constexpr bool supported = false;
};

template <class T>
struct ValueTraits<T, std::enable_if_t<internal::is_supported_value_type_v<T>>>
{
    static constexpr bool supported = true;
    using value_type = internal::canonical_storage_type_t<T>;

    static constexpr value_type to_storage(T value)
    {
        return static_cast<value_type>(value);
    }

    static constexpr T from_storage(value_type value)
    {
        return static_cast<T>(value);
    }
};

template <class T>
inline constexpr bool has_value_traits_v = ValueTraits<internal::remove_cvref_t<T>>::supported;

} // namespace string_math
