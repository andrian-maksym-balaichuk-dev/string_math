#pragma once
// Do not include this file directly. Use <string_math/string_math.hpp> or individual public headers.

#include <fw/function_ref.hpp>
#include <fw/function_wrapper.hpp>
#include <fw/move_only_function_wrapper.hpp>

#if defined(_MSVC_LANG)
#define STRING_MATH_CXX_STANDARD _MSVC_LANG
#else
#define STRING_MATH_CXX_STANDARD __cplusplus
#endif

#if STRING_MATH_CXX_STANDARD >= 202302L
#define STRING_MATH_HAS_CXX23 1
#else
#define STRING_MATH_HAS_CXX23 0
#endif

#if STRING_MATH_CXX_STANDARD >= 202002L
#define STRING_MATH_HAS_CXX20 1
#define STRING_MATH_HAS_CONSTEXPR_EVALUATION 1
#define STRING_MATH_HAS_CONSTEVAL 1
#else
#define STRING_MATH_HAS_CXX20 0
#define STRING_MATH_HAS_CONSTEXPR_EVALUATION 0
#define STRING_MATH_HAS_CONSTEVAL 0
#endif

#define STRING_MATH_ALL_VALUE_TYPES \
    short, unsigned short, int, unsigned int, long, unsigned long, long long, unsigned long long, float, double, long double

#define STRING_MATH_SIGNED_AND_FLOATING_VALUE_TYPES short, int, long, long long, float, double, long double

#define STRING_MATH_UNSIGNED_INTEGRAL_VALUE_TYPES unsigned short, unsigned int, unsigned long, unsigned long long

#define STRING_MATH_SIGNED_FACTORIAL_SIGNATURES short(short), int(int), long(long), long long(long long)

#define STRING_MATH_UNSIGNED_FACTORIAL_SIGNATURES \
    unsigned short(unsigned short), unsigned int(unsigned int), unsigned long(unsigned long), unsigned long long(unsigned long long)

#define STRING_MATH_ALL_UNARY_SAME_TYPE_SIGNATURES \
    short(short), unsigned short(unsigned short), int(int), unsigned int(unsigned int), long(long), unsigned long(unsigned long), \
        long long(long long), unsigned long long(unsigned long long), float(float), double(double), long double(long double)

#define STRING_MATH_ALL_BINARY_SAME_TYPE_SIGNATURES \
    short(short, short), unsigned short(unsigned short, unsigned short), int(int, int), unsigned int(unsigned int, unsigned int), \
        long(long, long), unsigned long(unsigned long, unsigned long), long long(long long, long long), \
        unsigned long long(unsigned long long, unsigned long long), float(float, float), double(double, double), \
        long double(long double, long double)
