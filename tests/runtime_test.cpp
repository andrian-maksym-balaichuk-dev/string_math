#include <fx/math.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using fx::operator""_math;

namespace
{

bool approx_equal(double left, double right, double epsilon = 1.0e-9)
{
    return std::fabs(left - right) <= epsilon;
}

bool contains_name(const std::vector<std::string>& values, const std::string& name)
{
    return std::find(values.begin(), values.end(), name) != values.end();
}

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
int add_runtime_shaped_two(int value)
{
    return value + 2;
}

int prefix_runtime_shaped_double(int value)
{
    return value * 2;
}

int infix_runtime_shaped_join(int left, int right)
{
    return left + right;
}

int runtime_identity(int value)
{
    return value;
}

int runtime_join_two(int left, int right)
{
    return left + right;
}
#endif

} // namespace

int main()
{
    try
    {
        fx::Calculator calculator;

        const fx::MathValue int_sum = calculator.evaluate("1 + 2");
        require(int_sum.is<int>(), "1 + 2 should stay int");
        require(int_sum.get<int>() == 3, "1 + 2 should equal 3");

        const fx::MathValue double_sum = calculator.evaluate("1.0 + 2");
        require(double_sum.is<double>(), "1.0 + 2 should deduce double");
        require(approx_equal(double_sum.get<double>(), 3.0), "1.0 + 2 should equal 3.0");

        const fx::MathValue float_sum = calculator.evaluate("1f + 3");
        require(float_sum.is<float>(), "1f + 3 should deduce float");
        require(approx_equal(float_sum.get<float>(), 4.0f), "1f + 3 should equal 4f");

        const fx::MathValue long_sum = calculator.evaluate("1l + 2l");
        require(long_sum.is<long>(), "1l + 2l should deduce long");
        require(long_sum.get<long>() == 3L, "1l + 2l should equal 3l");

        const fx::MathValue unsigned_long_sum = calculator.evaluate("1ul + 2ul");
        require(unsigned_long_sum.is<unsigned long>(), "1ul + 2ul should deduce unsigned long");
        require(unsigned_long_sum.get<unsigned long>() == 3UL, "1ul + 2ul should equal 3ul");

        const fx::MathValue uint8_literal = calculator.evaluate("255u8");
        require(uint8_literal.is<unsigned short>(), "255u8 should normalize to unsigned short storage");
        require(uint8_literal.get<unsigned short>() == static_cast<unsigned short>(255), "255u8 should preserve value");

        const fx::MathValue int16_literal = calculator.evaluate("42i16");
        require(int16_literal.is<std::int16_t>(), "42i16 should deduce int16_t");
        require(int16_literal.get<std::int16_t>() == static_cast<std::int16_t>(42), "42i16 should preserve int16_t");

        const fx::MathValue size_literal = calculator.evaluate("7uz");
        require(size_literal.is<std::size_t>(), "7uz should deduce size_t");
        require(size_literal.get<std::size_t>() == static_cast<std::size_t>(7), "7uz should preserve size_t");

        const fx::MathValue literal_udl = "3 + 2"_math;
        require(literal_udl.is<int>(), "\"3 + 2\"_math should evaluate");
        require(literal_udl.get<int>() == 5, "\"3 + 2\"_math should equal 5");

        const fx::MathValue prefixed = calculator.evaluate("0x10 + 0b10");
        require(prefixed == 18, "hex and binary literals should parse");
        require(calculator.evaluate("3 > 2").get<int>() == 1, "comparison operators should evaluate");
        require(calculator.evaluate("3 < 2").get<int>() == 0, "comparison operators should return false as 0");
        require(calculator.evaluate("1 && 0").get<int>() == 0, "logical and should evaluate");
        require(calculator.evaluate("1 || 0").get<int>() == 1, "logical or should evaluate");
        require(calculator.evaluate("!0").get<int>() == 1, "prefix logical not should evaluate");
        require(calculator.evaluate("1 ? 4 : 9").get<int>() == 4, "ternary should pick true branch");
        require(calculator.evaluate("0 ? 4 : 9").get<int>() == 9, "ternary should pick false branch");
        require((fx::MathValue(1) + fx::MathValue(2)).is<int>(), "MathValue operator+ should preserve int");
        require((fx::MathValue(1) + fx::MathValue(2)) == 3, "MathValue operator+ should work");
        require((fx::MathValue(5) % fx::MathValue(2)) == 1, "MathValue operator% should work");
        require(fx::MathValue(5) > fx::MathValue(2), "MathValue comparisons should work");

        std::ostringstream stream;
        stream << fx::MathValue(4.0f);
        require(stream.str() == "4f", "MathValue stream output should be available");

        {
            fx::MathContext named_context;
            const std::string runtime_name = "runtime_value";
            const std::string_view view_name = "view_value";
            const std::string runtime_function = "id_from_string";
            const std::string_view view_function = "id_from_view";
            const std::string runtime_symbol = "sum_from_string";
            const std::string_view view_symbol = "sum_from_view";

            named_context.set_value(runtime_name, 7);
            named_context.set_value(view_name, 8);
            named_context.register_function(runtime_function, [](int value) { return value; });
            named_context.register_function(view_function, [](int value) { return value; });
            named_context.register_infix_operator(runtime_symbol, [](int left, int right) { return left + right; }, fx::Precedence::Additive);
            named_context.register_infix_operator(view_symbol, [](int left, int right) { return left + right; }, fx::Precedence::Additive);

            require(
                fx::evaluate("{runtime_value}", named_context).get<int>() == 7,
                "runtime MathContext should accept std::string variable names");
            require(
                fx::evaluate("{view_value}", named_context).get<int>() == 8,
                "runtime MathContext should accept std::string_view variable names");
            require(
                fx::evaluate("id_from_string(3)", named_context).get<int>() == 3,
                "runtime MathContext should accept std::string function names");
            require(
                fx::evaluate("id_from_view(4)", named_context).get<int>() == 4,
                "runtime MathContext should accept std::string_view function names");
            require(
                fx::evaluate("1 sum_from_string 2", named_context).get<int>() == 3,
                "runtime MathContext should accept std::string operator symbols");
            require(
                fx::evaluate("1 sum_from_view 2", named_context).get<int>() == 3,
                "runtime MathContext should accept std::string_view operator symbols");
        }

        calculator.set_value("a", 2);
        calculator.set_value("b", 3U);
        calculator.set_value("s", static_cast<short>(4));
        const fx::MathValue variables = calculator.evaluate("{a} + {b}");
        require(variables.is<unsigned int>(), "mixed int and unsigned should select unsigned overload");
        require(variables.get<unsigned int>() == 5U, "variable expression should evaluate");
        const fx::MathValue short_variables = calculator.evaluate("{s} + {s}");
        require(short_variables.is<int>(), "short operands should be supported and promote to int");
        require(short_variables.get<int>() == 8, "short variable expression should evaluate");

        const auto parse_error = calculator.try_evaluate("1 + )");
        require(!parse_error.has_value(), "try_evaluate should expose parse failures");
        require(parse_error.error().kind() == fx::ErrorKind::Parse, "parse failure should be classified");
        require(parse_error.error().span().begin == 4, "parse failure should include span");

        const auto missing_variable = calculator.try_evaluate("{missing} + 1");
        require(!missing_variable.has_value(), "try_evaluate should expose evaluation failures");
        require(missing_variable.error().kind() == fx::ErrorKind::NameResolution, "missing variable should be classified");
        require(missing_variable.error().token() == "missing", "missing variable should report token");

        fx::Calculator snapshot_source;
        snapshot_source.set_value("frozen_value", 11);
        const auto frozen = snapshot_source.snapshot();
        snapshot_source.set_value("frozen_value", 99);
        require(fx::evaluate("{frozen_value}", frozen).get<int>() == 11, "snapshot should preserve visible values");

        const auto function_names = calculator.context().function_names();
        require(contains_name(function_names, "sin"), "introspection should list builtin functions");
        const auto plus_info = calculator.context().inspect_infix_operator("+");
        require(plus_info.has_value(), "introspection should inspect infix operators");
        require(plus_info->precedence == fx::Precedence::Additive, "introspection should expose precedence");
        require(
            !plus_info->binary_overloads.empty() &&
                plus_info->binary_overloads.front().semantics.arithmetic_kind == fx::ArithmeticKind::Add,
            "introspection should expose overload semantics");

        const fx::MathContext built_context = fx::MathContext::with_builtins()
                                                  .with_value("builder_value", 12)
                                                  .with_infix_operator("join", [](int left, int right) { return left + right; }, fx::Precedence::Additive);
        require(fx::evaluate("{builder_value}", built_context).get<int>() == 12, "with_value should build immutable copies");
        require(fx::evaluate("1 join 2", built_context).get<int>() == 3, "with_infix_operator should build immutable copies");

        fx::EvaluationPolicy floating_policy;
        floating_policy.promotion = fx::PromotionPolicy::WidenToFloating;
        fx::MathContext floating_context = fx::MathContext::with_builtins().with_policy(floating_policy);
        require(fx::evaluate("1 + 2", floating_context).is<double>(), "widen-to-floating policy should prefer floating overloads");

        fx::Calculator narrowing_calculator;
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

        fx::MathContext checked_context = fx::MathContext::with_builtins();
        fx::EvaluationPolicy checked_policy;
        checked_policy.overflow = fx::OverflowPolicy::Checked;
        checked_context.set_policy(checked_policy);
        checked_context.add_infix_operator<unsigned int(unsigned int, unsigned int)>(
            "plus_meta",
            [](unsigned int left, unsigned int right) { return left + right; },
            fx::Precedence::Additive,
            fx::Associativity::Left,
            fx::OperatorSemantics::arithmetic_add());
        checked_context.add_infix_operator<unsigned int(unsigned int, unsigned int)>(
            "plus_raw",
            [](unsigned int left, unsigned int right) { return left + right; },
            fx::Precedence::Additive,
            fx::Associativity::Left);
        checked_context.add_infix_operator<unsigned int(unsigned int, unsigned int)>(
            "times_policy",
            [](unsigned int left, unsigned int right) { return left * right; },
            fx::Precedence::Multiplicative,
            fx::Associativity::Left,
            {},
            fx::BinaryPolicyKind::IntegralMultiplyOverflow);
        const auto meta_overflow = fx::try_evaluate("4294967295u plus_meta 1u", checked_context);
        require(!meta_overflow.has_value(), "metadata-marked custom add should participate in overflow policy");
        require(meta_overflow.error().kind() == fx::ErrorKind::Evaluation, "checked overflow should be evaluation error");
        require(fx::evaluate("4294967295u plus_raw 1u", checked_context).get<unsigned int>() == 0u, "custom operator without semantics should use raw callable behavior");
        const auto direct_policy_overflow = fx::try_evaluate("65536u times_policy 65536u", checked_context);
        require(!direct_policy_overflow.has_value(), "explicit binary policy argument should participate in overflow checks");
        require(direct_policy_overflow.error().kind() == fx::ErrorKind::Evaluation, "explicit binary policy should report overflow");
        checked_context.add_infix_operator_with_policy<unsigned int(unsigned int, unsigned int)>(
            "plus_custom",
            [](unsigned int left, unsigned int right) { return left * right; },
            fx::Precedence::Additive,
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
            });
        const auto custom_policy_overflow = fx::try_evaluate("65536u plus_custom 65536u", checked_context);
        require(!custom_policy_overflow.has_value(), "custom policy callback should be available when adding operators");
        require(custom_policy_overflow.error().token() == "plus_custom", "custom policy should receive operator token");
        checked_context.add_infix_operator<unsigned int(unsigned int, unsigned int)>(
            "+",
            [](unsigned int left, unsigned int right) { return left * right; },
            fx::Precedence::Additive,
            fx::Associativity::Left,
            fx::OperatorSemantics::arithmetic_multiply());
        const auto multiply_overflow = fx::try_evaluate("65536u + 65536u", checked_context);
        require(!multiply_overflow.has_value(), "overflow policy should come from registered overload metadata, not the operator symbol");
        require(multiply_overflow.error().kind() == fx::ErrorKind::Evaluation, "custom '+' with multiply policy should still report overflow");

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

        calculator.context().add_infix_operator(
            "avg", [](double left, double right) { return (left + right) / 2.0; }, fx::Precedence::Additive, fx::Associativity::Left);
        require(approx_equal(calculator.evaluate("8 avg 4").get<double>(), 6.0), "custom infix operator should evaluate");

        calculator.context().add_function("cos", [](double) { return 42.0; });
        require(approx_equal(calculator.evaluate("cos(1.5)").get<double>(), 42.0), "custom function should override builtin");
        require(calculator.evaluate("max(1, 9, 3)").get<int>() == 9, "builtin variadic max should evaluate");
        calculator.context().add_function("answer", []() { return 42; });
        calculator.context().add_function("sum3", [](int a, int b, int c) { return a + b + c; });
        calculator.context().add_function("sum6", [](int a, int b, int c, int d, int e, int f) { return a + b + c + d + e + f; });
        require(calculator.evaluate("answer()").get<int>() == 42, "fixed zero-arity function should evaluate");
        require(calculator.evaluate("sum3(1, 2, 3)").get<int>() == 6, "fixed ternary function should evaluate");
        require(calculator.evaluate("sum6(1, 2, 3, 4, 5, 6)").get<int>() == 21, "fixed 6-argument function should evaluate");
        const auto sum3_info = calculator.context().inspect_function("sum3");
        require(sum3_info.has_value(), "introspection should inspect fixed-arity functions");
        require(
            !sum3_info->fixed_overloads.empty() && sum3_info->fixed_overloads.front().argument_types.size() == 3,
            "introspection should expose fixed-arity function signatures");
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
        require(calculator.evaluate("sum(1, 2, 3, 4)").get<int>() == 10, "custom variadic function should evaluate");

        fx::Calculator alias_calculator;
        alias_calculator.context().register_variable("x", 5);
        alias_calculator.context().register_function("id", [](int value) { return value; });
        alias_calculator.context().register_infix_operator(
            "sum", [](int left, int right) { return left + right; }, fx::Precedence::Additive, fx::Associativity::Left);
        require(alias_calculator.evaluate("id({x})").get<int>() == 5, "configured calculator context should resolve registered functions");
        require(alias_calculator.evaluate("1 sum 2").get<int>() == 3, "configured calculator context should resolve registered operators");

        const fx::MathContext free_builtin_context = fx::with_builtins(fx::MathContext{});
        require(fx::evaluate("{PI} + 1", free_builtin_context).is<double>(), "free with_builtins should configure runtime contexts");

        calculator.context().add_infix_operator_for<int, float, double>(
            "mean", [](auto left, auto right) { return (left + right) / static_cast<decltype(left)>(2); },
            fx::Precedence::Additive, fx::Associativity::Left);
        require(calculator.evaluate("6 mean 2").get<int>() == 4, "multi-type infix helper should register int overload");
        require(approx_equal(calculator.evaluate("5.0 mean 3.0").get<double>(), 4.0), "multi-type infix helper should register double overload");

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
        fx::MathContext templated_context;
        templated_context.set_value("templated_value", 4);
        templated_context.add_function<runtime_identity>("id2");
        templated_context.add_infix_operator<runtime_join_two>("join2", fx::Precedence::Additive);
        require(
            fx::evaluate("id2({templated_value}) join2 3", templated_context).get<int>() == 7,
            "runtime context should support the same name-parameter builder style in c++20+");

        fx::Calculator fixed_string_calculator;
        fixed_string_calculator.context().register_function<int(int), add_runtime_shaped_two>("plus_two");
        fixed_string_calculator.context().register_prefix_operator<int(int), prefix_runtime_shaped_double>("dbl");
        fixed_string_calculator.context().register_infix_operator<int(int, int), infix_runtime_shaped_join>(
            "join",
            fx::Precedence::Additive);
        require(
            fixed_string_calculator.evaluate("plus_two(3)").get<int>() == 5,
            "Calculator fixed-string function registration should support explicit Sig overloads");
        require(
            fixed_string_calculator.evaluate("dbl 4").get<int>() == 8,
            "Calculator fixed-string prefix registration should support explicit Sig overloads");
        require(
            fixed_string_calculator.evaluate("1 join 2").get<int>() == 3,
            "Calculator fixed-string infix registration should support explicit Sig overloads");

        fx::MathExpr fixed_string_expr("1");
        fixed_string_expr.mutable_context().register_function<int(int), add_runtime_shaped_two>("plus_two");
        fixed_string_expr.mutable_context().register_prefix_operator<int(int), prefix_runtime_shaped_double>("dbl");
        fixed_string_expr.mutable_context().register_infix_operator<int(int, int), infix_runtime_shaped_join>(
            "join",
            fx::Precedence::Additive);
        require(
            fixed_string_expr.context().find_function("plus_two") != nullptr,
            "MathExpr fixed-string function registration should update local context");
        require(
            fixed_string_expr.context().find_prefix_operator("dbl") != nullptr,
            "MathExpr fixed-string prefix registration should update local context");
        require(
            fixed_string_expr.context().find_infix_operator("join") != nullptr,
            "MathExpr fixed-string infix registration should update local context");
#endif

        calculator.context().add_literal_parser("", "kg", [](std::string_view token) -> std::optional<fx::MathValue> {
            const std::string body(token.substr(0, token.size() - 2));
            return fx::MathValue(static_cast<long long>(std::stoll(body) * 1000LL));
        });
        require(calculator.evaluate("2kg + 500").get<long long>() == 2500LL, "custom literal suffix should parse");

        const fx::MathValue factorial = calculator.evaluate("5!");
        require(factorial.is<int>(), "factorial should preserve int result for int input");
        require(factorial.get<int>() == 120, "factorial should evaluate");

        fx::MathExpr expression("{x} + {PI}");
        expression.substitute("x", 2);
        const auto expression_result = expression.try_eval();
        require(expression_result.has_value(), "MathExpr::try_eval should succeed for valid expressions");
        require(expression_result.value().get<double>() > 5.0, "MathExpr::try_eval should return a value");
        const std::vector<std::string> names = expression.variables();
        require(contains_name(names, "x"), "variables() should include local variable");
        require(contains_name(names, "PI"), "variables() should include inherited variable");
        const std::vector<std::string> local_names = expression.local_variables();
        require(contains_name(local_names, "x"), "local_variables() should include substituted variable");

        fx::MathExpr power_expression("2 + 3");
        power_expression.mutable_context().add_infix_operator<double(double, double)>(
            "+", [](double left, double right) { return std::pow(left, right); }, fx::Precedence::Additive, fx::Associativity::Left);
        require(approx_equal(power_expression.eval().get<double>(), 8.0), "local operator override should affect expression evaluation");

        const auto compiled = fx::MathExpr("{value} + 2").compile<double>("value");
        const fx::MathValue compiled_result = compiled(5.0);
        require(compiled_result.is<double>(), "compiled callable should preserve result type");
        require(approx_equal(compiled_result.get<double>(), 7.0), "compiled callable should evaluate");

        fx::SMath::set_value("global_counter", 9);
        require(fx::SMath::evaluate("{global_counter} + 1") == 10, "SMath should expose global evaluation");
        require(fx::evaluate("1 + 2").is<int>(), "free evaluate should work for string literals");

        std::cout << "runtime tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
