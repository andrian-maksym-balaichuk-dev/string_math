#pragma once
// Do not include this file directly. Use <string_math/string_math.hpp> or individual public headers.

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <string_math/internal/overload_impl.hpp>
#include <string_math/semantics/overload.hpp>
#include <string_math/value/policy.hpp>

namespace string_math::internal
{

struct LiteralParserEntry
{
    std::string prefix;
    std::string suffix;
    fw::function_wrapper<std::optional<MathValue>(std::string_view)> parser;
};

struct PrefixOperatorEntry
{
    int precedence{ Precedence::Prefix };
    std::vector<UnaryOverload> overloads;
};

struct PostfixOperatorEntry
{
    int precedence{ Precedence::Postfix };
    std::vector<UnaryOverload> overloads;
};

struct InfixOperatorEntry
{
    int precedence{ Precedence::Additive };
    Associativity associativity{ Associativity::Left };
    std::vector<BinaryOverload> overloads;
};

struct FunctionEntry
{
    std::vector<FunctionOverload> fixed_overloads;
    struct VariadicOverload
    {
        std::size_t min_arity{1};
        CallableSemantics semantics{};
        std::shared_ptr<const void> storage;
        MathValue (*invoke)(const void*, const std::vector<MathValue>&) = nullptr;
    };
    std::vector<VariadicOverload> variadic_overloads;
};

struct RuntimeContextData
{
    std::shared_ptr<RuntimeContextData> m_parent;
    std::unordered_map<std::string, MathValue> m_values;
    std::unordered_map<std::string, FunctionEntry> m_functions;
    std::unordered_map<std::string, PrefixOperatorEntry> m_prefix_operators;
    std::unordered_map<std::string, PostfixOperatorEntry> m_postfix_operators;
    std::unordered_map<std::string, InfixOperatorEntry> m_infix_operators;
    std::vector<LiteralParserEntry> m_literal_parsers;
    std::vector<std::string> m_prefix_symbols;
    std::vector<std::string> m_postfix_symbols;
    std::vector<std::string> m_infix_symbols;
    EvaluationPolicy m_policy{};
};

} // namespace string_math::internal
