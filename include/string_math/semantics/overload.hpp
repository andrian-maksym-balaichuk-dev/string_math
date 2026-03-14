#pragma once

#include <string>
#include <vector>

#include <string_math/internal/catalog.hpp>

namespace string_math
{

struct CallableOverloadInfo
{
    ValueType result_type{};
    internal::ArityKind arity_kind{internal::ArityKind::Fixed};
    std::size_t min_arity{0};
    std::size_t max_arity{0};
    std::vector<ValueType> argument_types;
    CallableSemantics semantics{};
};

struct FunctionInfo
{
    std::string name;
    std::vector<CallableOverloadInfo> overloads;
};

struct OperatorInfo
{
    std::string symbol;
    int precedence{0};
    Associativity associativity{Associativity::Left};
    std::vector<CallableOverloadInfo> overloads;
};

struct LiteralParserInfo
{
    std::string prefix;
    std::string suffix;
};

} // namespace string_math
