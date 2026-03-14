#include <string_math/builtin/default_math.hpp>
#include <string_math/string_math.hpp>

using string_math::evaluate;
using string_math::operator""_math;

namespace
{

constexpr int k_additive = string_math::builtin::default_math::precedence::additive;
constexpr int k_prefix = string_math::builtin::default_math::precedence::prefix;
constexpr int k_postfix = string_math::builtin::default_math::precedence::postfix;

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

constexpr int answer()
{
    return 42;
}

constexpr int sum3(int a, int b, int c)
{
    return a + b + c;
}

constexpr int sum6(int a, int b, int c, int d, int e, int f)
{
    return a + b + c + d + e + f;
}

} // namespace

static_assert(evaluate("1 + 2").is<int>(), "1 + 2 should stay int at compile time");
static_assert(evaluate("1 + 2").get<int>() == 3, "1 + 2 should equal 3");
static_assert(evaluate("1.0 + 2").is<double>(), "1.0 + 2 should deduce double");
static_assert(evaluate("255u8").is<unsigned short>(), "255u8 should normalize to unsigned short storage");
static_assert(evaluate("(1 + 2) * 3").get<int>() == 9, "compile-time precedence should work");
static_assert(evaluate("-2 + 5").get<int>() == 3, "compile-time unary minus should work");
static_assert(evaluate("3 > 2").get<int>() == 1, "compile-time comparisons should work");
static_assert(evaluate("1 && 0").get<int>() == 0, "compile-time logical and should work");
static_assert(evaluate("!0").get<int>() == 1, "compile-time logical not should work");
static_assert(evaluate("0 ? 4 : 9").get<int>() == 9, "compile-time ternary should work");
static_assert(evaluate("max(1, 9, 3)").get<int>() == 9, "compile-time builtin variadic max should work");
static_assert("3 + 2"_math.get<int>() == 5, "builtin UDL should evaluate at compile time");

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
constexpr auto builtin_context = string_math::builtin::default_math_compile_time_context();
static_assert(evaluate("{PI} + 1", builtin_context).get<double>() > 4.0, "explicit builtin compile-time context should expose constants");

constexpr auto custom_context = string_math::builtin::with_default_math(string_math::MathContext::compile_time()).set_value("value", 5);
static_assert(evaluate("{value} + 3", custom_context).get<int>() == 8, "constexpr custom variables should work");

constexpr auto custom_infix_context =
    string_math::MathContext::compile_time().add_infix_operator<custom_plus_one>("plus1", k_additive);
static_assert(evaluate("2 plus1 3", custom_infix_context).get<int>() == 6, "constexpr custom infix operators should work");

constexpr auto custom_prefix_context =
    string_math::MathContext::compile_time().add_prefix_operator<custom_double>("dbl", k_prefix);
static_assert(evaluate("dbl 4", custom_prefix_context).get<int>() == 8, "constexpr custom prefix operators should work");

constexpr auto custom_postfix_context =
    string_math::MathContext::compile_time().add_postfix_operator<custom_increment>("++", k_postfix);
static_assert(evaluate("4++", custom_postfix_context).get<int>() == 5, "constexpr custom postfix operators should work");

constexpr auto custom_function_context =
    string_math::MathContext::compile_time().add_function<custom_square>("square");
static_assert(evaluate("square(5)", custom_function_context).get<int>() == 25, "constexpr custom functions should work");

constexpr auto fixed_arity_context = string_math::builtin::with_default_math(string_math::MathContext::compile_time())
                                         .add_function<answer>("answer")
                                         .add_function<sum3>("sum3")
                                         .add_function<sum6>("sum6");
static_assert(evaluate("answer()", fixed_arity_context).get<int>() == 42, "constexpr fixed zero-arity functions should work");
static_assert(evaluate("sum3(1, 2, 3)", fixed_arity_context).get<int>() == 6, "constexpr fixed ternary functions should work");
static_assert(evaluate("sum6(1, 2, 3, 4, 5, 6)", fixed_arity_context).get<int>() == 21, "constexpr fixed 6-argument functions should work");

constexpr auto chained_custom_context = string_math::builtin::with_default_math(string_math::MathContext::compile_time())
                                            .add_variable("a", 2)
                                            .add_infix_operator<custom_plus_one>("sum1", k_additive)
                                            .add_function<custom_square>("square");
static_assert(evaluate("square({a} sum1 3)", chained_custom_context).get<int>() == 36, "constexpr chaining should match runtime style");
#endif

int main()
{
    return 0;
}
