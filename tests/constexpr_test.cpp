#include <string_math/string_math.hpp>

using string_math::evaluate;
using string_math::operator""_math;

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

int main()
{
    return 0;
}
