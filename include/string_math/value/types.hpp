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

} // namespace string_math
