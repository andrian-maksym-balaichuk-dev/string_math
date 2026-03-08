#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <string_math/result.hpp>
#include <string_math/value.hpp>

namespace string_math::detail
{

struct UnaryOverload
{
    ValueType result_type{};
    ValueType argument_type{};
    std::shared_ptr<const void> storage;
    MathValue (*invoke)(const void*, const MathValue&) = nullptr;
};

struct BinaryOverload
{
    ValueType result_type{};
    ValueType left_type{};
    ValueType right_type{};
    std::shared_ptr<const void> storage;
    MathValue (*invoke)(const void*, const MathValue&, const MathValue&) = nullptr;
};

template <class Sig, class F>
inline UnaryOverload make_unary_overload(F&& function)
{
    using traits = signature_traits<Sig>;
    static_assert(traits::arity == 1, "unary overload requires one parameter");
    using result_t = typename traits::result_type;
    using argument_t = std::tuple_element_t<0, typename traits::args_tuple>;
    static_assert(is_supported_value_type_v<result_t>, "unsupported result type");
    static_assert(is_supported_value_type_v<argument_t>, "unsupported argument type");

    using wrapper_t = fw::function_wrapper<Sig>;
    auto storage = std::make_shared<wrapper_t>(std::forward<F>(function));

    return UnaryOverload{
        value_type_of<result_t>::value,
        value_type_of<argument_t>::value,
        storage,
        [](const void* raw_storage, const MathValue& argument) {
            const auto& wrapper = *static_cast<const wrapper_t*>(raw_storage);
            return MathValue(wrapper(argument.template cast<argument_t>()));
        },
    };
}

template <class Sig, class F>
inline BinaryOverload make_binary_overload(F&& function)
{
    using traits = signature_traits<Sig>;
    static_assert(traits::arity == 2, "binary overload requires two parameters");
    using result_t = typename traits::result_type;
    using left_t = std::tuple_element_t<0, typename traits::args_tuple>;
    using right_t = std::tuple_element_t<1, typename traits::args_tuple>;
    static_assert(is_supported_value_type_v<result_t>, "unsupported result type");
    static_assert(is_supported_value_type_v<left_t>, "unsupported left type");
    static_assert(is_supported_value_type_v<right_t>, "unsupported right type");

    using wrapper_t = fw::function_wrapper<Sig>;
    auto storage = std::make_shared<wrapper_t>(std::forward<F>(function));

    return BinaryOverload{
        value_type_of<result_t>::value,
        value_type_of<left_t>::value,
        value_type_of<right_t>::value,
        storage,
        [](const void* raw_storage, const MathValue& left, const MathValue& right) {
            const auto& wrapper = *static_cast<const wrapper_t*>(raw_storage);
            return MathValue(wrapper(left.template cast<left_t>(), right.template cast<right_t>()));
        },
    };
}

template <class Overload>
void upsert_overload(std::vector<Overload>& overloads, Overload overload);

template <>
inline void upsert_overload(std::vector<UnaryOverload>& overloads, UnaryOverload overload)
{
    const auto found = std::find_if(overloads.begin(), overloads.end(), [&](const UnaryOverload& current) {
        return current.argument_type == overload.argument_type && current.result_type == overload.result_type;
    });

    if (found != overloads.end())
    {
        *found = std::move(overload);
        return;
    }

    overloads.push_back(std::move(overload));
}

template <>
inline void upsert_overload(std::vector<BinaryOverload>& overloads, BinaryOverload overload)
{
    const auto found = std::find_if(overloads.begin(), overloads.end(), [&](const BinaryOverload& current) {
        return current.left_type == overload.left_type && current.right_type == overload.right_type &&
            current.result_type == overload.result_type;
    });

    if (found != overloads.end())
    {
        *found = std::move(overload);
        return;
    }

    overloads.push_back(std::move(overload));
}

struct ResolutionError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

inline Result<const UnaryOverload*>
try_resolve_unary_overload(const std::vector<UnaryOverload>& overloads, ValueType argument_type, std::string_view symbol)
{
    const UnaryOverload* best = nullptr;
    int best_cost = std::numeric_limits<int>::max();
    bool ambiguous = false;

    for (const auto& overload : overloads)
    {
        const int cost = conversion_cost(argument_type, overload.argument_type);
        if (cost >= 1'000'000)
        {
            continue;
        }

        if (cost < best_cost)
        {
            best = &overload;
            best_cost = cost;
            ambiguous = false;
        }
        else if (cost == best_cost)
        {
            ambiguous = true;
        }
    }

    if (best == nullptr)
    {
        return Error(
            ErrorKind::OverloadResolution,
            "string_math: no matching unary overload for '" + std::string(symbol) + "'",
            {},
            std::string(symbol));
    }

    if (ambiguous)
    {
        return Error(
            ErrorKind::OverloadResolution,
            "string_math: ambiguous unary overload for '" + std::string(symbol) + "'",
            {},
            std::string(symbol));
    }

    return best;
}

inline const UnaryOverload&
resolve_unary_overload(const std::vector<UnaryOverload>& overloads, ValueType argument_type, std::string_view symbol)
{
    return *try_resolve_unary_overload(overloads, argument_type, symbol).value();
}

inline Result<const BinaryOverload*> try_resolve_binary_overload(
    const std::vector<BinaryOverload>& overloads,
    ValueType left_type,
    ValueType right_type,
    std::string_view symbol,
    PromotionPolicy policy = PromotionPolicy::CppLike)
{
    const BinaryOverload* best = nullptr;
    int best_cost = std::numeric_limits<int>::max();
    bool ambiguous = false;
    const ValueType preferred_target = preferred_binary_target(left_type, right_type, policy);

    for (const auto& overload : overloads)
    {
        const int left_cost = conversion_cost(left_type, overload.left_type);
        const int right_cost = conversion_cost(right_type, overload.right_type);
        if (left_cost >= 1'000'000 || right_cost >= 1'000'000)
        {
            continue;
        }

        const int preferred_bonus = overload.left_type == preferred_target && overload.right_type == preferred_target ? -50 : 0;
        const int total_cost = left_cost + right_cost + preferred_bonus;
        if (total_cost < best_cost)
        {
            best = &overload;
            best_cost = total_cost;
            ambiguous = false;
        }
        else if (total_cost == best_cost)
        {
            ambiguous = true;
        }
    }

    if (best == nullptr)
    {
        return Error(
            ErrorKind::OverloadResolution,
            "string_math: no matching binary overload for '" + std::string(symbol) + "'",
            {},
            std::string(symbol));
    }

    if (ambiguous)
    {
        return Error(
            ErrorKind::OverloadResolution,
            "string_math: ambiguous binary overload for '" + std::string(symbol) + "'",
            {},
            std::string(symbol));
    }

    return best;
}

inline const BinaryOverload&
resolve_binary_overload(
    const std::vector<BinaryOverload>& overloads,
    ValueType left_type,
    ValueType right_type,
    std::string_view symbol,
    PromotionPolicy policy = PromotionPolicy::CppLike)
{
    return *try_resolve_binary_overload(overloads, left_type, right_type, symbol, policy).value();
}

inline long double factorial_value(long double value)
{
    if (value < 0.0L)
    {
        throw std::domain_error("string_math: factorial requires a non-negative value");
    }

    const long double rounded = std::round(value);
    if (std::fabs(static_cast<double>(rounded - value)) > 1.0e-9)
    {
        throw std::domain_error("string_math: factorial requires an integral value");
    }

    long double result = 1.0L;
    for (long long index = 2; index <= static_cast<long long>(rounded); ++index)
    {
        result *= static_cast<long double>(index);
    }
    return result;
}

} // namespace string_math::detail
