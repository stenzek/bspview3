#pragma once
#include <cmath>
#include <cstdint>

using u8 = uint8_t;
using s8 = int8_t;
using u16 = uint16_t;
using s16 = int16_t;
using u32 = uint32_t;
using s32 = int32_t;
using u64 = uint64_t;
using s64 = int64_t;

// https://www.g-truc.net/post-0708.html
#ifndef __has_feature
#define __has_feature(x) 0 // Compatibility with non-clang compilers.
#endif
// Any compiler claiming C++11 supports, Visual C++ 2015 and Clang version supporting constexp
#if __cplusplus >= 201103L || _MSC_VER >= 1900 || __has_feature(cxx_constexpr) // C++ 11 implementation
namespace detail {
template<typename T, std::size_t N>
constexpr std::size_t countof(T const (&)[N]) noexcept
{
  return N;
}
} // namespace detail
#define ARRAY_SIZE(arr) detail::countof(arr)
#elif _MSC_VER // Visual C++ fallback
#define ARRAY_SIZE(arr) _countof(arr)
#elif __cplusplus >= 199711L && ( // C++ 98 trick
defined(__INTEL_COMPILER) || defined(__clang__) ||
  (defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4))) template<typename T, std::size_t N>
  char (&COUNTOF_REQUIRES_ARRAY_ARGUMENT(T (&)[N]))[N];
#define countof(x) sizeof(COUNTOF_REQUIRES_ARRAY_ARGUMENT(x))
#else
#define ARRAY_SIZE(arr) sizeof(arr) / sizeof(arr[0])
#endif

template<typename T>
constexpr bool NearEqual(T lhs, T rhs, T epsilon)
{
  return std::abs(lhs - rhs) <= epsilon;
}
