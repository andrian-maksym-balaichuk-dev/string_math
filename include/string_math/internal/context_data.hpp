#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <string_math/internal/catalog.hpp>
#include <string_math/internal/overload_impl.hpp>

namespace string_math::internal
{

struct LiteralParserEntry
{
    std::string prefix;
    std::string suffix;
    fw::function_wrapper<std::optional<MathValue>(std::string_view)> parser;
};

struct CallableEntry
{
    int precedence{0};
    Associativity associativity{Associativity::Left};
    std::vector<RuntimeCallableOverload> overloads;

    CollectedCallable collect() const
    {
        CollectedCallable collected;
        collected.precedence = precedence;
        collected.associativity = associativity;
        collected.overloads.reserve(overloads.size());
        for (const auto& overload : overloads)
        {
            collected.overloads.push_back(overload.view());
        }
        return collected;
    }
};

using PrefixOperatorEntry = CallableEntry;
using PostfixOperatorEntry = CallableEntry;
using InfixOperatorEntry = CallableEntry;
using FunctionEntry = CallableEntry;

struct RuntimeContextData
{
    std::shared_ptr<RuntimeContextData> m_parent;
    std::unordered_map<std::string, MathValue> m_values;
    std::unordered_map<std::string, FunctionEntry> m_functions;
    std::unordered_map<std::string, PrefixOperatorEntry> m_prefix_operators;
    std::unordered_map<std::string, PostfixOperatorEntry> m_postfix_operators;
    std::unordered_map<std::string, InfixOperatorEntry> m_infix_operators;
    std::vector<LiteralParserEntry> m_literal_parsers;
    EvaluationPolicy m_policy{};
};

} // namespace string_math::internal
