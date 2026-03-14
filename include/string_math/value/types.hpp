#pragma once

#include <string_math/internal/config.hpp>

namespace string_math
{

enum class ValueType
{
    Short,
    UnsignedShort,
    Int,
    UnsignedInt,
    Long,
    UnsignedLong,
    LongLong,
    UnsignedLongLong,
    Float,
    Double,
    LongDouble
};

enum class Associativity
{
    Left,
    Right
};

struct Precedence
{
    using value_type = int;

    static constexpr value_type Conditional = 1;
    static constexpr value_type LogicalOr = 2;
    static constexpr value_type LogicalAnd = 3;
    static constexpr value_type Comparison = 5;
    static constexpr value_type Addition = 10;
    static constexpr value_type Additive = Addition;
    static constexpr value_type Multiplication = 20;
    static constexpr value_type Multiplicative = Multiplication;
    static constexpr value_type Exponentiation = 30;
    static constexpr value_type Power = Exponentiation;
    static constexpr value_type UnaryPrefix = 40;
    static constexpr value_type Prefix = UnaryPrefix;
    static constexpr value_type UnaryPostfix = 50;
    static constexpr value_type Postfix = UnaryPostfix;
};

} // namespace string_math
