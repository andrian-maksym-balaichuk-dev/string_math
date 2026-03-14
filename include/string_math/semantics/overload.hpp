#pragma once

#include <string>
#include <vector>

#include <string_math/value/types.hpp>
#include <string_math/semantics/semantics.hpp>

namespace string_math
{

struct UnaryOverloadInfo
{
    ValueType result_type{};
    ValueType argument_type{};
    CallableSemantics semantics{};
};

struct BinaryOverloadInfo
{
    ValueType result_type{};
    ValueType left_type{};
    ValueType right_type{};
    CallableSemantics semantics{};
};

struct FunctionInfo
{
    std::string name;
    std::vector<UnaryOverloadInfo> unary_overloads;
    std::vector<BinaryOverloadInfo> binary_overloads;
    std::vector<std::size_t> variadic_min_arities;
    std::vector<CallableSemantics> variadic_semantics;
};

struct OperatorInfo
{
    std::string symbol;
    int precedence{0};
    Associativity associativity{Associativity::Left};
    std::vector<UnaryOverloadInfo> unary_overloads;
    std::vector<BinaryOverloadInfo> binary_overloads;
};

struct LiteralParserInfo
{
    std::string prefix;
    std::string suffix;
};

} // namespace string_math
