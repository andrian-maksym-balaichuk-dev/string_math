#include <string_math/string_math.hpp>

using string_math::evaluate;
using string_math::operator""_math;

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
constexpr int custom_plus_one(int left, int right)
{
    return left + right + 1;
}

constexpr int custom_double(int value)
{
    return value * 2;
}

constexpr int custom_increment(int value)
{
    return value + 1;
}

constexpr int custom_square(int value)
{
    return value * value;
}
#endif

static_assert(evaluate("1 + 2").is<int>(), "1 + 2 should stay int at compile time");
static_assert(evaluate("1 + 2").get<int>() == 3, "1 + 2 should equal 3");
static_assert(evaluate("1.0 + 2").is<double>(), "1.0 + 2 should deduce double");
static_assert(evaluate("1f + 3").is<float>(), "1f + 3 should deduce float");
static_assert(evaluate("1l + 2l").is<long>(), "1l + 2l should deduce long");
static_assert(evaluate("1ul + 2ul").is<unsigned long>(), "1ul + 2ul should deduce unsigned long");
static_assert(evaluate("255u8").is<unsigned short>(), "255u8 should normalize to unsigned short storage");
static_assert(evaluate("42i16").is<std::int16_t>(), "42i16 should deduce int16_t");
static_assert(evaluate("7uz").is<std::size_t>(), "7uz should deduce size_t");
static_assert(evaluate("(1 + 2) * 3").is<int>(), "parentheses should preserve integer result");
static_assert(evaluate("(1 + 2) * 3").get<int>() == 9, "compile-time precedence should work");
static_assert(evaluate("-2 + 5").get<int>() == 3, "compile-time unary minus should work");
static_assert(evaluate("3 > 2").get<int>() == 1, "compile-time comparisons should work");
static_assert(evaluate("1 && 0").get<int>() == 0, "compile-time logical and should work");
static_assert(evaluate("!0").get<int>() == 1, "compile-time logical not should work");
static_assert(evaluate("0 ? 4 : 9").get<int>() == 9, "compile-time ternary should work");
static_assert("3 + 2"_math.get<int>() == 5, "\"3 + 2\"_math should evaluate at compile time");

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
constexpr auto custom_context = string_math::ConstexprContext<>().with_value<"value">(5);
static_assert(evaluate("{value} + 3", custom_context).get<int>() == 8, "constexpr custom variables should work");

constexpr auto custom_infix_context = string_math::ConstexprContext<>().with_infix_operator<"plus1", custom_plus_one>(
    string_math::Precedence::Additive);
static_assert(evaluate("2 plus1 3", custom_infix_context).get<int>() == 6, "constexpr custom infix operators should work");

constexpr auto custom_prefix_context = string_math::ConstexprContext<>().with_prefix_operator<"dbl", custom_double>();
static_assert(evaluate("dbl 4", custom_prefix_context).get<int>() == 8, "constexpr custom prefix operators should work");

constexpr auto custom_postfix_context = string_math::ConstexprContext<>().with_postfix_operator<"++", custom_increment>();
static_assert(evaluate("4++", custom_postfix_context).get<int>() == 5, "constexpr custom postfix operators should work");

constexpr auto custom_function_context = string_math::ConstexprContext<>().with_function<"square", custom_square>();
static_assert(evaluate("square(5)", custom_function_context).get<int>() == 25, "constexpr custom functions should work");
#endif

int main()
{
    return 0;
}
