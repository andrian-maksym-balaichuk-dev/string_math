#pragma once

namespace string_math
{

enum class OverflowPolicy
{
    Wrap,
    Checked,
    Saturate,
    Promote
};

enum class PromotionPolicy
{
    CppLike,
    PreserveType,
    WidenToFloating
};

enum class NarrowingPolicy
{
    Allow,
    Checked
};

struct EvaluationPolicy
{
    OverflowPolicy overflow{OverflowPolicy::Wrap};
    PromotionPolicy promotion{PromotionPolicy::CppLike};
    NarrowingPolicy narrowing{NarrowingPolicy::Allow};
};

} // namespace string_math
