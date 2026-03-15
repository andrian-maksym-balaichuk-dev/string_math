#include <fx/math.hpp>
#include <string_math/builtin/default_math.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using fx::builtin::operator""_math;

namespace
{

constexpr int k_additive = fx::builtin::default_math::precedence::additive;
constexpr int k_multiplicative = fx::builtin::default_math::precedence::multiplicative;
constexpr int k_prefix = fx::builtin::default_math::precedence::prefix;
constexpr int k_postfix = fx::builtin::default_math::precedence::postfix;

bool approx_equal(double left, double right, double epsilon = 1.0e-9)
{
    return std::fabs(left - right) <= epsilon;
}

bool contains_name(const std::vector<std::string>& values, const std::string& name)
{
    return std::find(values.begin(), values.end(), name) != values.end();
}

int typed_variadic_sum(const std::vector<int>& arguments)
{
    int total = 0;
    for (const int argument : arguments)
    {
        total += argument;
    }
    return total;
}

fx::MathValue raw_variadic_sum(const fx::MathValue* arguments, std::size_t count)
{
    int total = 0;
    for (std::size_t index = 0; index < count; ++index)
    {
        total += arguments[index].get<int>();
    }
    return fx::MathValue(total);
}

fx::MathValue raw_identity_function(const fx::MathValue* arguments, std::size_t count)
{
    if (count != 1)
    {
        throw std::runtime_error("raw_identity_function arity mismatch");
    }
    return arguments[0];
}

fx::MathValue raw_add_operator(const fx::MathValue* arguments, std::size_t count)
{
    if (count != 2)
    {
        throw std::runtime_error("raw_add_operator arity mismatch");
    }
    return fx::MathValue(arguments[0].get<int>() + arguments[1].get<int>());
}

int policy_function_impl(int left, int right)
{
    return left + right + 100;
}

int policy_function_handler(int left, int right, const fx::EvaluationPolicy&, std::string_view)
{
    return left + right;
}

using sum_policy_signature = int(int, int);

const fx::RawMathCallable raw_identity_wrapper{raw_identity_function};
const fx::RawMathCallable raw_add_wrapper{raw_add_operator};

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
int runtime_identity(int value)
{
    return value;
}

int runtime_join_two(int left, int right)
{
    return left + right;
}

int runtime_prefix_double(int value)
{
    return value * 2;
}
#endif

} // namespace

int main()
{
    try
    {
        {
            const fx::MathContext empty_context;
            const auto no_constants = fx::try_evaluate("{PI} + 1", empty_context);
            require(!no_constants.has_value(), "empty core context must not expose builtin constants or operators");
        }

        fx::Calculator calculator(fx::builtin::default_math_context());

        const fx::MathValue int_sum = calculator.evaluate("1 + 2");
        require(int_sum.is<int>(), "1 + 2 should stay int");
        require(int_sum.get<int>() == 3, "1 + 2 should equal 3");

        const fx::MathValue double_sum = calculator.evaluate("1.0 + 2");
        require(double_sum.is<double>(), "1.0 + 2 should deduce double");
        require(approx_equal(double_sum.get<double>(), 3.0), "1.0 + 2 should equal 3");

        const fx::MathValue float_sum = calculator.evaluate("1f + 3");
        require(float_sum.is<float>(), "1f + 3 should deduce float");
        require(approx_equal(float_sum.get<float>(), 4.0f), "1f + 3 should equal 4");

        const fx::MathValue uint8_literal = calculator.evaluate("255u8");
        require(uint8_literal.is<unsigned short>(), "255u8 should normalize to unsigned short storage");
        require(uint8_literal.get<unsigned short>() == static_cast<unsigned short>(255), "255u8 should preserve value");

        const fx::MathValue int16_literal = calculator.evaluate("42i16");
        require(int16_literal.is<std::int16_t>(), "42i16 should deduce int16_t");
        require(int16_literal.get<std::int16_t>() == static_cast<std::int16_t>(42), "42i16 should preserve value");

        const fx::MathValue size_literal = calculator.evaluate("7uz");
        require(size_literal.is<std::size_t>(), "7uz should deduce size_t");
        require(size_literal.get<std::size_t>() == static_cast<std::size_t>(7), "7uz should preserve value");

        const fx::MathValue udl_result = "3 + 2"_math;
        require(udl_result.is<int>(), "builtin UDL should evaluate with explicit builtin header");
        require(udl_result.get<int>() == 5, "builtin UDL should equal 5");

        require(calculator.evaluate("0x10 + 0b10") == 18, "hex and binary literals should parse");
        require(calculator.evaluate("3 > 2").get<int>() == 1, "comparison operators should evaluate");
        require(calculator.evaluate("1 && 0").get<int>() == 0, "logical and should evaluate");
        require(calculator.evaluate("1 || 0").get<int>() == 1, "logical or should evaluate");
        require(calculator.evaluate("!0").get<int>() == 1, "prefix logical not should evaluate");
        require(calculator.evaluate("1 ? 4 : 9").get<int>() == 4, "ternary should pick true branch");
        require(calculator.evaluate("0 ? 4 : 9").get<int>() == 9, "ternary should pick false branch");
        require(calculator.evaluate("{PI} + 1").get<double>() > 4.0, "explicit builtin opt-in should expose constants");
        require(calculator.evaluate("max(1, 9, 3)").get<int>() == 9, "builtin variadic max should evaluate");
        require(calculator.evaluate("5!").get<int>() == 120, "postfix factorial should evaluate");

        std::ostringstream stream;
        stream << fx::MathValue(4.0f);
        require(stream.str() == "4f", "MathValue stream output should be available");

        {
            fx::MathContext named_context = fx::builtin::default_math_context();
            const std::string runtime_name = "runtime_value";
            const std::string_view view_name = "view_value";
            const std::string runtime_function = "id_from_string";
            const std::string_view view_function = "id_from_view";
            const std::string runtime_symbol = "sum_from_string";
            const std::string_view view_symbol = "sum_from_view";

            named_context.set_value(runtime_name, 7);
            named_context.set_value(view_name, 8);
            named_context.add_function(runtime_function, [](int value) { return value; });
            named_context.add_function(view_function, [](int value) { return value; });
            named_context.add_infix_operator(runtime_symbol, [](int left, int right) { return left + right; }, k_additive);
            named_context.add_infix_operator(view_symbol, [](int left, int right) { return left + right; }, k_additive);

            require(fx::evaluate("{runtime_value}", named_context).get<int>() == 7, "runtime context should accept std::string names");
            require(fx::evaluate("{view_value}", named_context).get<int>() == 8, "runtime context should accept std::string_view names");
            require(fx::evaluate("id_from_string(3)", named_context).get<int>() == 3, "runtime context should accept function names");
            require(fx::evaluate("id_from_view(4)", named_context).get<int>() == 4, "runtime context should accept string_view function names");
            require(fx::evaluate("1 sum_from_string 2", named_context).get<int>() == 3, "runtime context should accept operator strings");
            require(fx::evaluate("1 sum_from_view 2", named_context).get<int>() == 3, "runtime context should accept operator string_views");
        }

        calculator.set_value("a", 2);
        calculator.set_value("b", 3U);
        calculator.set_value("s", static_cast<short>(4));
        require(calculator.evaluate("{a} + {b}").get<unsigned int>() == 5U, "variable expression should evaluate");
        require(calculator.evaluate("{s} + {s}").get<int>() == 8, "short operands should promote and evaluate");

        const auto parse_error = calculator.try_evaluate("1 + )");
        require(!parse_error.has_value(), "try_evaluate should expose parse failures");
        require(parse_error.error().kind() == fx::ErrorKind::Parse, "parse failure should be classified");

        const auto missing_variable = calculator.try_evaluate("{missing} + 1");
        require(!missing_variable.has_value(), "try_evaluate should expose missing variables");
        require(missing_variable.error().kind() == fx::ErrorKind::NameResolution, "missing variable should be classified");
        require(missing_variable.error().token() == "missing", "missing variable should report token");

        fx::Calculator snapshot_source(fx::builtin::default_math_context());
        snapshot_source.set_value("frozen_value", 11);
        const auto frozen = snapshot_source.snapshot();
        snapshot_source.set_value("frozen_value", 99);
        require(fx::evaluate("{frozen_value}", frozen).get<int>() == 11, "snapshot should preserve visible values");

        const auto function_names = calculator.context().function_names();
        require(contains_name(function_names, "sin"), "function introspection should list builtin functions");
        const auto plus_info = calculator.context().inspect_infix_operator("+");
        require(plus_info.has_value(), "introspection should inspect infix operators");
        require(plus_info->precedence == k_additive, "introspection should expose precedence");
        require(!plus_info->overloads.empty(), "introspection should expose overloads");
        require(plus_info->overloads.front().semantics.has_flag(fx::CallableFlag::OverflowSensitive), "builtin '+' should expose overflow-sensitive semantics");

        const fx::MathContext built_context = fx::builtin::with_default_math(fx::MathContext{})
                                                  .with_value("builder_value", 12)
                                                  .with_infix_operator("join", [](int left, int right) { return left + right; }, k_additive);
        require(fx::evaluate("{builder_value}", built_context).get<int>() == 12, "with_value should build immutable copies");
        require(fx::evaluate("1 join 2", built_context).get<int>() == 3, "with_infix_operator should build immutable copies");

        fx::EvaluationPolicy floating_policy;
        floating_policy.promotion = fx::PromotionPolicy::WidenToFloating;
        fx::MathContext floating_context = fx::builtin::default_math_context().with_policy(floating_policy);
        require(fx::evaluate("1 + 2", floating_context).is<double>(), "widen-to-floating policy should prefer floating overloads");

        fx::Calculator narrowing_calculator(fx::builtin::default_math_context());
        fx::EvaluationPolicy narrowing_policy;
        narrowing_policy.narrowing = fx::NarrowingPolicy::Checked;
        narrowing_calculator.context().set_policy(narrowing_policy);
        bool narrowing_threw = false;
        try
        {
            (void)narrowing_calculator.evaluate_as<short>("70000");
        }
        catch (const std::out_of_range&)
        {
            narrowing_threw = true;
        }
        require(narrowing_threw, "checked narrowing should reject out-of-range conversions");

        fx::MathContext checked_context = fx::builtin::default_math_context();
        fx::EvaluationPolicy checked_policy;
        checked_policy.overflow = fx::OverflowPolicy::Checked;
        checked_context.set_policy(checked_policy);

        const auto builtin_overflow = fx::try_evaluate("4294967295u + 1u", checked_context);
        require(!builtin_overflow.has_value(), "builtin '+' should use policy hooks for overflow");
        require(builtin_overflow.error().kind() == fx::ErrorKind::Evaluation, "builtin overflow should be an evaluation error");

        checked_context.add_infix_operator<unsigned int(unsigned int, unsigned int)>(
            "plus_raw",
            [](unsigned int left, unsigned int right) { return left + right; },
            k_additive,
            fx::Associativity::Left);
        require(fx::evaluate("4294967295u plus_raw 1u", checked_context).get<unsigned int>() == 0u, "custom operator without policy hook should use raw callable behavior");

        checked_context.add_infix_operator_with_policy<unsigned int(unsigned int, unsigned int)>(
            "times_policy",
            [](unsigned int left, unsigned int right) { return left * right; },
            k_multiplicative,
            [](unsigned int left, unsigned int right, const fx::EvaluationPolicy& policy, std::string_view token)
                -> fx::Result<fx::MathValue> {
                const auto product = static_cast<unsigned long long>(left) * static_cast<unsigned long long>(right);
                if (policy.overflow == fx::OverflowPolicy::Checked &&
                    product > static_cast<unsigned long long>(std::numeric_limits<unsigned int>::max()))
                {
                    return fx::Error(
                        fx::ErrorKind::Evaluation,
                        "string_math: custom operator overflow",
                        {},
                        std::string(token));
                    }
                return fx::MathValue(static_cast<unsigned int>(left * right));
            },
            fx::Associativity::Left,
            fx::CallableSemantics(static_cast<std::uint32_t>(fx::CallableFlag::OverflowSensitive)));
        const auto direct_policy_overflow = fx::try_evaluate("65536u times_policy 65536u", checked_context);
        require(!direct_policy_overflow.has_value(), "custom policy callback should participate in overflow checks");
        require(direct_policy_overflow.error().token() == "times_policy", "custom policy should receive the operator token");

        checked_context.add_infix_operator_with_policy<unsigned int(unsigned int, unsigned int)>(
            "+",
            [](unsigned int left, unsigned int right) { return left * right; },
            k_additive,
            [](unsigned int left, unsigned int right, const fx::EvaluationPolicy& policy, std::string_view token)
                -> fx::Result<fx::MathValue> {
                const auto product = static_cast<unsigned long long>(left) * static_cast<unsigned long long>(right);
                if (policy.overflow == fx::OverflowPolicy::Checked &&
                    product > static_cast<unsigned long long>(std::numeric_limits<unsigned int>::max()))
                {
                    return fx::Error(
                        fx::ErrorKind::Evaluation,
                        "string_math: custom operator overflow",
                        {},
                        std::string(token));
                    }
                return fx::MathValue(static_cast<unsigned int>(left * right));
            },
            fx::Associativity::Left,
            fx::CallableSemantics(static_cast<std::uint32_t>(fx::CallableFlag::OverflowSensitive)));
        const auto custom_plus_overflow = fx::try_evaluate("65536u + 65536u", checked_context);
        require(!custom_plus_overflow.has_value(), "policy hooks should come from the registered overload, not the symbol");

        const fx::Operation cached = calculator.create_operation("{a} * 4");
        require(calculator.evaluate(cached) == 8, "cached operation should use current variable values");
        calculator.set_value("a", 5);
        require(calculator.evaluate(cached) == 20, "cached operation should be reusable");

        const auto parsed = fx::try_parse_operation("2 + 3 * 4", calculator.context());
        require(parsed.has_value(), "explicit parse stage should succeed");
        const auto bound = fx::try_bind_operation(parsed.value(), calculator.snapshot());
        require(bound.has_value(), "explicit bind stage should succeed");
        const auto optimized = fx::try_optimize_operation(bound.value());
        require(optimized.has_value(), "explicit optimize stage should succeed");
        require(fx::evaluate(optimized.value(), bound.value().context).get<int>() == 14, "optimized operation should evaluate");

        calculator.context().add_infix_operator("avg", [](double left, double right) { return (left + right) / 2.0; }, k_additive, fx::Associativity::Left);
        require(approx_equal(calculator.evaluate("8 avg 4").get<double>(), 6.0), "custom infix operator should evaluate");

        calculator.context().add_function("answer", []() { return 42; });
        calculator.context().add_function("sum3", [](int a, int b, int c) { return a + b + c; });
        calculator.context().add_function("sum6", [](int a, int b, int c, int d, int e, int f) { return a + b + c + d + e + f; });
        calculator.context().add_variadic_function(
            "sum",
            1,
            [](const std::vector<fx::MathValue>& arguments) {
                int total = 0;
                for (const auto& argument : arguments)
                {
                    total += argument.get<int>();
                }
                return fx::MathValue(total);
            });
        require(calculator.evaluate("answer()").get<int>() == 42, "fixed zero-arity function should evaluate");
        require(calculator.evaluate("sum3(1, 2, 3)").get<int>() == 6, "fixed ternary function should evaluate");
        require(calculator.evaluate("sum6(1, 2, 3, 4, 5, 6)").get<int>() == 21, "fixed 6-argument function should evaluate");
        require(calculator.evaluate("sum(1, 2, 3, 4)").get<int>() == 10, "custom variadic function should evaluate");

        fx::MathContext parity_context;
        parity_context.add_variadic_function_for<int, typed_variadic_sum>("sumv", 1, 4);
        parity_context.add_variadic_function("sumraw", 1, 3, fx::RawMathCallable{raw_variadic_sum}, fx::ValueType::Int);
        parity_context.add_infix_operator_with_policy<sum_policy_signature, policy_function_impl, policy_function_handler>(
            "sum_policy",
            k_additive);
        require(fx::evaluate("sumv(1, 2, 3, 4)", parity_context).get<int>() == 10, "typed bounded variadic registration should work");
        require(fx::evaluate("sumraw(1, 2, 3)", parity_context).get<int>() == 6, "raw variadic trampoline registration should work");
        require(fx::evaluate("2 sum_policy 3", parity_context).get<int>() == 5, "policy-aware infix registration should share the same user shape");

        fx::MathContext erased_context;
        erased_context.add_function("gid", fx::RawMathCallable{raw_identity_function}, 1, 1, fx::ValueType::Int);
        erased_context.add_infix_operator(
            "gplus",
            k_additive,
            fx::Associativity::Left,
            fx::RawMathCallable{raw_add_operator},
            fx::ValueType::Int);
        require(
            fx::evaluate("gid(7) gplus 5", erased_context).get<int>() == 12,
            "generic erased callable registration should work through the same shared wrapper-backed path");

        fx::MathContext wrapped_erased_context;
        wrapped_erased_context.add_function("wid", raw_identity_wrapper, 1, 1, fx::ValueType::Int);
        wrapped_erased_context.add_infix_operator(
            "wplus",
            k_additive,
            fx::Associativity::Left,
            raw_add_wrapper,
            fx::ValueType::Int);
        require(
            fx::evaluate("wid(7) wplus 5", wrapped_erased_context).get<int>() == 12,
            "runtime generic registration should also accept fw::function_wrapper");

        fx::MathContext wrapped_typed_context;
        wrapped_typed_context.add_function(
            "triplew",
            fw::function_wrapper<int(int)>{[](int value) { return value * 3; }});
        wrapped_typed_context.add_function(
            "move_id",
            fw::move_only_function_wrapper<int(int)>{[](int value) { return value; }});
        wrapped_typed_context.add_infix_operator(
            "avgw",
            fw::function_wrapper<double(double, double)>{[](double left, double right) { return (left + right) / 2.0; }},
            k_additive,
            fx::Associativity::Left);
        wrapped_typed_context.add_infix_operator_with_policy(
            "sum_policy_wrap",
            fw::function_wrapper<int(int, int)>{policy_function_impl},
            k_additive,
            fw::function_wrapper<int(int, int, const fx::EvaluationPolicy&, std::string_view)>{
                policy_function_handler},
            fx::Associativity::Left,
            fx::CallableSemantics(static_cast<std::uint32_t>(fx::CallableFlag::OverflowSensitive)));
        require(fx::evaluate("triplew(4)", wrapped_typed_context).get<int>() == 12, "typed fw::function_wrapper registration should work");
        require(fx::evaluate("move_id(9)", wrapped_typed_context).get<int>() == 9, "typed fw::move_only_function_wrapper registration should work");
        require(
            approx_equal(fx::evaluate("8 avgw 4", wrapped_typed_context).get<double>(), 6.0),
            "typed wrapper registration should work for infix operators");
        require(
            fx::evaluate("2 sum_policy_wrap 3", wrapped_typed_context).get<int>() == 5,
            "typed wrapper registration should also work for policy-aware infix operators");

        const auto sum3_info = calculator.context().inspect_function("sum3");
        require(sum3_info.has_value(), "introspection should inspect fixed-arity functions");
        require(!sum3_info->overloads.empty(), "function introspection should expose overloads");
        require(sum3_info->overloads.front().min_arity == 3, "fixed-arity function should expose its arity");
        require(sum3_info->overloads.front().argument_types.size() == 3, "fixed-arity function should expose its argument types");

        fx::Calculator alias_calculator;
        alias_calculator.context().set_value("x", 5);
        alias_calculator.context().add_function("id", [](int value) { return value; });
        alias_calculator.context().add_infix_operator("sum", [](int left, int right) { return left + right; }, k_additive, fx::Associativity::Left);
        require(alias_calculator.evaluate("id({x})").get<int>() == 5, "empty core context should still support explicit registrations");
        require(alias_calculator.evaluate("1 sum 2").get<int>() == 3, "explicit registrations should resolve operators");

        calculator.context().add_infix_operator_for<int, float, double>(
            "mean",
            [](auto left, auto right) { return (left + right) / static_cast<decltype(left)>(2); },
            k_additive,
            fx::Associativity::Left);
        require(calculator.evaluate("6 mean 2").get<int>() == 4, "multi-type infix helper should register int overload");
        require(approx_equal(calculator.evaluate("5.0 mean 3.0").get<double>(), 4.0), "multi-type infix helper should register double overload");

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
        fx::MathContext templated_context = fx::builtin::default_math_context();
        templated_context.set_value("templated_value", 4);
        templated_context.add_function<runtime_identity>("id2");
        templated_context.add_infix_operator<runtime_join_two>("join2", k_additive);
        templated_context.add_prefix_operator<runtime_prefix_double>("dbl", k_prefix);
        require(fx::evaluate("id2({templated_value}) join2 3", templated_context).get<int>() == 7, "runtime context should support explicit function-pointer builders");
        require(fx::evaluate("dbl 4", templated_context).get<int>() == 8, "runtime context should support explicit prefix registration");
#endif

        calculator.context().add_literal_parser("", "kg", [](std::string_view token) -> std::optional<fx::MathValue> {
            const std::string body(token.substr(0, token.size() - 2));
            return fx::MathValue(static_cast<long long>(std::stoll(body) * 1000LL));
        });
        require(calculator.evaluate("2kg + 500").get<long long>() == 2500LL, "custom literal suffix should parse");

        fx::MathExpr expression("{x} + {PI}", fx::builtin::default_math_context());
        expression.substitute("x", 2);
        require(expression.try_eval().value().get<double>() > 5.0, "MathExpr should evaluate with an explicit builtin context");
        require(contains_name(expression.variables(), "x"), "variables() should include local variables");
        require(contains_name(expression.variables(), "PI"), "variables() should include inherited variables");
        require(contains_name(expression.local_variables(), "x"), "local_variables() should include substituted variables");

        fx::MathExpr power_expression("2 + 3", fx::builtin::default_math_context());
        power_expression.mutable_context().add_infix_operator<double(double, double)>(
            "+",
            [](double left, double right) { return std::pow(left, right); },
            k_additive,
            fx::Associativity::Left);
        require(approx_equal(power_expression.eval().get<double>(), 8.0), "local operator overrides should affect expression evaluation");

        const auto compiled = fx::MathExpr("{value} + 2", fx::builtin::default_math_context()).compile<double>("value");
        require(approx_equal(compiled(5.0).get<double>(), 7.0), "compiled callables should evaluate");

        require(fx::builtin::evaluate("1 + 2").is<int>(), "builtin free evaluate should work when the builtin header is included");

        std::cout << "runtime tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
