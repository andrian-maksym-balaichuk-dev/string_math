#pragma once

#include <type_traits>

#include <string_math/calculator.hpp>
#include <string_math/internal/context/static_context.hpp>
#include <string_math/internal/parser/parser.hpp>
#include <string_math/internal/expression/operation.hpp>

namespace string_math
{

class MathExpr
{
public:
    explicit MathExpr(std::string expression, MathContext context)
    : m_expression(std::move(expression)), m_parent_context(std::move(context)),
      m_local_context(m_parent_context, true), m_operation(string_math::try_parse_operation(m_expression, m_local_context).value().operation)
    {}

    MathExpr& substitute(std::string name, MathValue value)
    {
        m_local_context.set_value(std::move(name), value);
        return *this;
    }

    Result<MathValue> try_eval() const
    {
        return try_evaluate_operation(m_operation, m_local_context);
    }

    Result<MathValue> try_result() const
    {
        return try_eval();
    }

    MathValue eval() const
    {
        return try_eval().value();
    }

    MathValue result() const
    {
        return eval();
    }

    template <class T>
    T result_as() const
    {
        return cast_value<T>(eval(), m_local_context.policy().narrowing);
    }

    std::vector<std::string> variables() const
    {
        return m_operation.variables();
    }

    std::vector<std::string> local_variables() const
    {
        std::vector<std::string> names;
        for (const auto& name : m_operation.variables())
        {
            const auto value = m_local_context.find_value(name);
            const auto parent_value = m_parent_context.find_value(name);
            if (value && (!parent_value || *value != *parent_value))
            {
                names.push_back(name);
            }
        }
        return names;
    }

    const MathContext& context() const noexcept
    {
        return m_local_context;
    }

    MathContext& mutable_context() noexcept
    {
        return m_local_context;
    }

    template <class... Ts, class... Names>
    auto compile(Names&&... names) const -> fw::function_wrapper<MathValue(Ts...)>
    {
        static_assert(sizeof...(Ts) == sizeof...(Names), "compile names/types mismatch");
        const auto operation = m_operation;
        const auto context = m_local_context;
        const std::array<std::string, sizeof...(Ts)> variable_names{ std::string(std::forward<Names>(names))... };
        return fw::function_wrapper<MathValue(Ts...)>([operation, context, variable_names](Ts... values) -> MathValue {
            MathContext call_context(context);
            std::size_t index = 0;
            (call_context.set_value(variable_names[index++], MathValue(values)), ...);
            return evaluate_operation(operation, call_context);
        });
    }

private:
    std::string m_expression;
    MathContext m_parent_context;
    MathContext m_local_context;
    Operation m_operation;
};

inline Result<MathValue> try_evaluate(std::string_view expression, const MathContext& context)
{
    const auto operation_result = string_math::try_create_operation(expression, context);
    if (!operation_result)
    {
        return operation_result.error();
    }
    return try_evaluate_operation(operation_result.value(), context);
}

inline Result<MathValue> try_evaluate(const Operation& operation, const MathContext& context)
{
    return try_evaluate_operation(operation, context);
}

inline MathValue evaluate(std::string_view expression, const MathContext& context)
{
    return try_evaluate(expression, context).value();
}

inline MathValue evaluate(const Operation& operation, const MathContext& context)
{
    return try_evaluate(operation, context).value();
}

#if STRING_MATH_HAS_CONSTEXPR_EVALUATION
template <class Context, class = void>
struct is_constexpr_context : std::false_type
{};

template <class Context>
struct is_constexpr_context<Context, std::void_t<decltype(Context::is_constexpr_context)>> : std::bool_constant<Context::is_constexpr_context>
{};

template <std::size_t N, class Context, std::enable_if_t<is_constexpr_context<Context>::value, int> = 0>
constexpr MathValue evaluate(const char (&expression)[N], const Context& context)
{
    return internal::ConstexprParser<Context>(std::string_view(expression, N - 1), &context).parse();
}

template <std::size_t N, class Context, std::enable_if_t<is_constexpr_context<Context>::value, int> = 0>
consteval MathValue evaluate_consteval(const char (&expression)[N], const Context& context)
{
    return internal::ConstexprParser<Context>(std::string_view(expression, N - 1), &context).parse();
}

// Runtime overload: evaluate a dynamic (non-literal) string with a compile-time context.
// This runs the ConstexprParser at runtime — the same registered functions are invoked
// via their function-pointer path, so no separate runtime registration is needed.
// Use this when the expression string is not a compile-time constant (e.g. user input).
template <class Context, std::enable_if_t<is_constexpr_context<Context>::value, int> = 0>
MathValue evaluate(std::string_view expression, const Context& context)
{
    return internal::ConstexprParser<Context>(expression, &context).parse();
}

// ---------------------------------------------------------------------------
// ConstexprCalculator<Context>
//
// A Calculator-like class wrapping a StaticMathContext. Provides the same
// evaluate() interface as Calculator, but:
//   - evaluate("literal", ...)  → constexpr: compile-time when the call is
//     in a constant expression context (constexpr/static_assert/template arg)
//   - evaluate(dynamic_string)  → always runtime
//
// Context modifications (add_function, set_value, add_*_operator) return a
// NEW ConstexprCalculator with the updated context — use `auto` for the result.
//
// Example:
//   constexpr auto calc = ConstexprCalculator<>()
//       .add_function("sq", [](int x) { return x*x; })
//       .set_value("k", 42);
//   constexpr auto r1 = calc.evaluate("sq(5)");  // compile-time
//   auto r2 = calc.evaluate("sq(5)");             // runtime
//   std::string s = "sq(5)";
//   auto r3 = calc.evaluate(s);                   // runtime, dynamic string
// ---------------------------------------------------------------------------
template <class Context>
class ConstexprCalculatorBase
{
public:
    constexpr ConstexprCalculatorBase() = default;
    constexpr explicit ConstexprCalculatorBase(Context ctx) : m_context(std::move(ctx)) {}

    // ── evaluation ──────────────────────────────────────────────────────────

    /// Evaluate a string literal. Compile-time when possible, runtime otherwise.
    template <std::size_t N>
    [[nodiscard]] constexpr MathValue evaluate(const char (&expression)[N]) const
    {
        return string_math::evaluate(expression, m_context);
    }

    /// Evaluate a dynamic (non-literal) string — always runtime.
    [[nodiscard]] MathValue evaluate(std::string_view expression) const
    {
        return string_math::evaluate(expression, m_context);
    }

    /// Force compile-time evaluation. Fails to compile if the call is not
    /// in a constant expression context.
    template <std::size_t N>
    [[nodiscard]] consteval MathValue evaluate_consteval(const char (&expression)[N]) const
    {
        return string_math::evaluate_consteval(expression, m_context);
    }

    // ── context modification (returns a new calculator) ──────────────────────

    template <class Sig, auto Function>
    [[nodiscard]] constexpr auto add_function(std::string_view name, CallableSemantics semantics = {}) const
    {
        return ConstexprCalculatorBase{m_context.template add_function<Sig, Function>(name, semantics)};
    }

    template <auto Function>
    [[nodiscard]] constexpr auto add_function(std::string_view name, CallableSemantics semantics = {}) const
    {
        return ConstexprCalculatorBase{m_context.template add_function<Function>(name, semantics)};
    }

    template <class F>
    [[nodiscard]] constexpr auto add_function(std::string_view name, F function, CallableSemantics semantics = {}) const
    {
        return ConstexprCalculatorBase{m_context.add_function(name, function, semantics)};
    }

    template <class Sig, auto Function>
    [[nodiscard]] constexpr auto add_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return ConstexprCalculatorBase{
            m_context.template add_infix_operator<Sig, Function>(symbol, precedence, associativity, semantics)};
    }

    template <auto Function>
    [[nodiscard]] constexpr auto add_infix_operator(
        std::string_view symbol,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return ConstexprCalculatorBase{
            m_context.template add_infix_operator<Function>(symbol, precedence, associativity, semantics)};
    }

    template <class F>
    [[nodiscard]] constexpr auto add_infix_operator(
        std::string_view symbol,
        F function,
        int precedence,
        Associativity associativity = Associativity::Left,
        CallableSemantics semantics = {}) const
    {
        return ConstexprCalculatorBase{
            m_context.add_infix_operator(symbol, function, precedence, associativity, semantics)};
    }

    template <class Sig, auto Function>
    [[nodiscard]] constexpr auto add_prefix_operator(
        std::string_view symbol, int precedence, CallableSemantics semantics = {}) const
    {
        return ConstexprCalculatorBase{
            m_context.template add_prefix_operator<Sig, Function>(symbol, precedence, semantics)};
    }

    template <auto Function>
    [[nodiscard]] constexpr auto add_prefix_operator(
        std::string_view symbol, int precedence, CallableSemantics semantics = {}) const
    {
        return ConstexprCalculatorBase{
            m_context.template add_prefix_operator<Function>(symbol, precedence, semantics)};
    }

    template <class F>
    [[nodiscard]] constexpr auto add_prefix_operator(
        std::string_view symbol, F function, int precedence, CallableSemantics semantics = {}) const
    {
        return ConstexprCalculatorBase{
            m_context.add_prefix_operator(symbol, function, precedence, semantics)};
    }

    template <class Sig, auto Function>
    [[nodiscard]] constexpr auto add_postfix_operator(
        std::string_view symbol, int precedence, CallableSemantics semantics = {}) const
    {
        return ConstexprCalculatorBase{
            m_context.template add_postfix_operator<Sig, Function>(symbol, precedence, semantics)};
    }

    template <auto Function>
    [[nodiscard]] constexpr auto add_postfix_operator(
        std::string_view symbol, int precedence, CallableSemantics semantics = {}) const
    {
        return ConstexprCalculatorBase{
            m_context.template add_postfix_operator<Function>(symbol, precedence, semantics)};
    }

    template <class F>
    [[nodiscard]] constexpr auto add_postfix_operator(
        std::string_view symbol, F function, int precedence, CallableSemantics semantics = {}) const
    {
        return ConstexprCalculatorBase{
            m_context.add_postfix_operator(symbol, function, precedence, semantics)};
    }

    [[nodiscard]] constexpr auto set_value(std::string_view name, MathValue value) const
    {
        return ConstexprCalculatorBase{m_context.set_value(name, value)};
    }

    template <class T>
    [[nodiscard]] constexpr auto set_value(std::string_view name, T value) const
    {
        return ConstexprCalculatorBase{m_context.set_value(name, MathValue(value))};
    }

    [[nodiscard]] constexpr std::optional<MathValue> find_value(std::string_view name) const
    {
        return m_context.find_value(name);
    }

    [[nodiscard]] constexpr const Context& context() const noexcept { return m_context; }

private:
    Context m_context;
};

/// Default ConstexprCalculator using the default-capacity StaticMathContext.
/// Context modifications return a NEW calculator — use `auto`.
using ConstexprCalculator = ConstexprCalculatorBase<
    internal::StaticMathContext<100, 100, 100, 100, 100>>;

/// Convenience factory — starts with the builtin math context (same as default_math_compile_time_context).
/// Returns a ConstexprCalculatorBase wrapping the builtin context.
#endif

inline MathExpr to_math_expr(std::string expression, const MathContext& context)
{
    return MathExpr(std::move(expression), context);
}

} // namespace string_math
