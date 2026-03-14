#pragma once
// Do not include this file directly. Use <string_math/string_math.hpp> or individual public headers.

#include <string_math/internal/config.hpp>
#include <string_math/value/types.hpp>

#include <tuple>
#include <type_traits>

namespace string_math::internal
{

template <class...>
inline constexpr bool always_false_v = false;

template <class Sig>
struct signature_traits;

template <class R, class... Args>
struct signature_traits<R(Args...)>
{
    using result_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <class R, class... Args>
struct signature_traits<R (*)(Args...)> : signature_traits<R(Args...)>
{};

template <class T>
struct remove_cvref
{
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template <class T>
using remove_cvref_t = typename remove_cvref<T>::type;

template <class T>
struct is_character_like : std::false_type
{};

template <>
struct is_character_like<char> : std::true_type
{};

template <>
struct is_character_like<wchar_t> : std::true_type
{};

template <>
struct is_character_like<char16_t> : std::true_type
{};

template <>
struct is_character_like<char32_t> : std::true_type
{};

#if STRING_MATH_HAS_CXX20
template <>
struct is_character_like<char8_t> : std::true_type
{};
#endif

template <class T>
inline constexpr bool is_character_like_v = is_character_like<remove_cvref_t<T>>::value;

template <class T>
inline constexpr bool is_supported_integral_input_v =
    std::is_integral_v<remove_cvref_t<T>> && !std::is_same_v<remove_cvref_t<T>, bool> && !is_character_like_v<T>;

template <class T>
struct type_identity
{
    using type = T;
};

} // namespace string_math::internal
