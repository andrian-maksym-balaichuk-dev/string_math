#include <string_math/builtin/default_math.hpp>
#include <string_math/string_math.hpp>

#include <type_traits>

using string_math::evaluate;
using string_math::builtin::evaluate;
using string_math::builtin::operator""_math;

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

constexpr int typed_variadic_sum(const std::vector<int>& arguments)
{
    int total = 0;
    for (const int argument : arguments)
    {
        total += argument;
    }
    return total;
}

constexpr string_math::MathValue raw_variadic_sum(
    const string_math::MathValue* arguments,
    std::size_t count)
{
    int total = 0;
    for (std::size_t index = 0; index < count; ++index)
    {
        total += arguments[index].get<int>();
    }
    return string_math::MathValue(total);
}

constexpr string_math::MathValue raw_identity_function(
    const string_math::MathValue* arguments,
    std::size_t count)
{
    return count == 1 ? arguments[0] : string_math::MathValue(0);
}

constexpr string_math::MathValue raw_add_operator(
    const string_math::MathValue* arguments,
    std::size_t count)
{
    return count == 2 ? string_math::MathValue(arguments[0].get<int>() + arguments[1].get<int>()) : string_math::MathValue(0);
}

constexpr int policy_function_impl(int left, int right)
{
    return left + right + 100;
}

constexpr int policy_function_handler(
    int left,
    int right,
    const string_math::EvaluationPolicy&,
    std::string_view)
{
    return left + right;
}

using sum_policy_signature = int(int, int);

constexpr string_math::RawMathCallable raw_identity_wrapper{raw_identity_function};
constexpr string_math::RawMathCallable raw_add_wrapper{raw_add_operator};
constexpr string_math::RawMathCallable raw_identity_lambda_wrapper{
        [](const string_math::MathValue* arguments, std::size_t count) constexpr -> string_math::MathValue {
            return count == 1 ? arguments[0] : string_math::MathValue(0);
        }};
constexpr string_math::RawMathCallable raw_add_lambda_wrapper{
        [](const string_math::MathValue* arguments, std::size_t count) constexpr -> string_math::MathValue {
            return count == 2
                ? string_math::MathValue(arguments[0].get<int>() + arguments[1].get<int>())
                : string_math::MathValue(0);
        }};

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

constexpr auto custom_context = string_math::builtin::default_math_compile_time_context().set_value("value", 5);
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

constexpr auto fixed_arity_context = string_math::builtin::default_math_compile_time_context()
                                         .add_function<answer>("answer")
                                         .add_function<sum3>("sum3")
                                         .add_function<sum6>("sum6");
static_assert(evaluate("answer()", fixed_arity_context).get<int>() == 42, "constexpr fixed zero-arity functions should work");
static_assert(evaluate("sum3(1, 2, 3)", fixed_arity_context).get<int>() == 6, "constexpr fixed ternary functions should work");
static_assert(evaluate("sum6(1, 2, 3, 4, 5, 6)", fixed_arity_context).get<int>() == 21, "constexpr fixed 6-argument functions should work");

constexpr auto chained_custom_context = string_math::builtin::default_math_compile_time_context()
                                            .set_value("a", 2)
                                            .add_infix_operator<custom_plus_one>("sum1", k_additive)
                                            .add_function<custom_square>("square");
static_assert(evaluate("square({a} sum1 3)", chained_custom_context).get<int>() == 36, "constexpr chaining should match runtime style");

constexpr auto parity_context = string_math::MathContext::compile_time()
                                    .add_variadic_function_for<int, typed_variadic_sum>("sumv", 1, 4)
                                    .add_variadic_function(
                                        "sumraw",
                                        1,
                                        3,
                                        string_math::RawMathCallable{raw_variadic_sum},
                                        string_math::ValueType::Int)
                                    .add_infix_operator_with_policy<
                                        sum_policy_signature,
                                        policy_function_impl,
                                        policy_function_handler>("sum_policy", k_additive);
static_assert(evaluate("sumv(1, 2, 3, 4)", parity_context).get<int>() == 10, "constexpr typed bounded variadic registration should work");
static_assert(evaluate("sumraw(1, 2, 3)", parity_context).get<int>() == 6, "constexpr raw variadic trampoline registration should work");
static_assert(evaluate("2 sum_policy 3", parity_context).get<int>() == 5, "constexpr policy-aware infix registration should match runtime shape");

constexpr auto erased_context = string_math::MathContext::compile_time()
                                    .add_function(
                                        "gid",
                                        string_math::RawMathCallable{raw_identity_function},
                                        1,
                                        1,
                                        string_math::ValueType::Int)
                                    .add_infix_operator(
                                        "gplus",
                                        k_additive,
                                        string_math::Associativity::Left,
                                        string_math::RawMathCallable{raw_add_operator},
                                        string_math::ValueType::Int);
static_assert(
    evaluate("gid(7) gplus 5", erased_context).get<int>() == 12,
    "constexpr generic erased callable registration should match runtime shape");

constexpr auto wrapped_erased_context = string_math::MathContext::compile_time()
                                            .add_function("wid", raw_identity_wrapper, 1, 1, string_math::ValueType::Int)
                                            .add_infix_operator(
                                                "wplus",
                                                k_additive,
                                                string_math::Associativity::Left,
                                                raw_add_wrapper,
                                                string_math::ValueType::Int);
static_assert(
    evaluate("wid(7) wplus 5", wrapped_erased_context).get<int>() == 12,
    "constexpr fw::function_wrapper registration should work for erased math callables");

constexpr auto wrapped_lambda_erased_context = string_math::MathContext::compile_time()
                                                   .add_function(
                                                       "lid",
                                                       raw_identity_lambda_wrapper,
                                                       1,
                                                       1,
                                                       string_math::ValueType::Int)
                                                   .add_infix_operator(
                                                       "lplus",
                                                       k_additive,
                                                       string_math::Associativity::Left,
                                                       raw_add_lambda_wrapper,
                                                       string_math::ValueType::Int);
static_assert(
    evaluate("lid(7) lplus 5", wrapped_lambda_erased_context).get<int>() == 12,
    "constexpr fw::function_wrapper registration should also work with noncapturing lambdas");

constexpr auto lambda_context = string_math::MathContext::compile_time()
                                    .add_function("triple", [](int value) { return value * 3; })
                                    .add_infix_operator("plus_lambda", [](int left, int right) { return left + right + 2; }, k_additive);
static_assert(
    std::is_same_v<
        std::remove_cv_t<decltype(string_math::MathContext::compile_time())>,
        std::remove_cv_t<decltype(lambda_context)>>,
    "constexpr add should keep the same context type");
static_assert(evaluate("triple(4)", lambda_context).get<int>() == 12, "constexpr direct lambda function registration should work");
static_assert(evaluate("2 plus_lambda 3", lambda_context).get<int>() == 7, "constexpr direct lambda operator registration should work");

constexpr auto small_capacity_context = string_math::MathContext::compile_time<1, 1, 1, 1, 1>()
                                            .add_function("id", [](int value) { return value; });
static_assert(evaluate("id(3)", small_capacity_context).get<int>() == 3, "custom constexpr capacity should still evaluate");
#endif

int main()
{
    return 0;
}
