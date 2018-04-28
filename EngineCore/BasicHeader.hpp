#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define WINPLATFORM
#endif

#define NOMINMAX

#ifdef WINPLATFORM
#include "WinAPI.hpp"
#endif

#define _USE_MATH_DEFINES
#include <cmath>

// these are still experimental on Android, iOS and Emscripten

#if __has_include(<optional>)
#include <optional>
using std::optional;
using std::nullopt;
#else
#warning The system doesn't provide unexperimental optional

#include <experimental/optional>
using std::experimental::optional;
using std::experimental::nullopt;
#endif

#if !__has_include(<variant>)
#warning The system doesn't provide <variant>
#endif

#include <string>
#include <string_view>
#include <vector>
#include <forward_list>
#include <cstdint>
#include <cassert>
#include <array>
#include <memory>
#include <type_traits>
#include <iterator>
#include <chrono>
#include <functional>
#include <variant>
#include <mutex>
#include <atomic>
#include <chrono>
#include <experimental\generator>
#include <experimental\resumable>

using std::string_view;
using std::make_unique;
using std::make_shared;
using std::make_pair;
using std::make_tuple;
using std::bind;
using std::pair;
using std::conditional_t;
using std::is_pod;
using std::static_pointer_cast;
using std::enable_if_t;
using std::initializer_list;
using std::string;
using std::vector;
using std::forward_list;
using std::array;
using std::function;
using std::weak_ptr;
using std::shared_ptr;
using std::tuple;
using std::unique_ptr;
using std::move;
using std::swap;
using std::to_string;
using std::clamp;
using std::variant;
using std::mutex;
using std::atomic;
namespace chrono = std::chrono;
using namespace std::literals::string_literals;

using uiw = uintptr_t;
using iw = intptr_t;
using ui64 = uint64_t;
using i64 = int64_t;
using ui32 = uint32_t;
using i32 = int32_t;
using ui16 = uint16_t;
using i16 = int16_t;
using ui8 = uint8_t;
using i8 = int8_t;
using f32 = float;
using f64 = double;
using byte = ui8;
using utf8char = char;
using utf32char = char32_t;
using string_view_utf8 = std::basic_string_view<utf8char>;
using string_view_utf32 = std::basic_string_view<utf32char>;
using string_utf8 = std::basic_string<utf8char>;
using string_utf32 = std::basic_string<utf32char>;

/* SOFTBREAK is an unexpected result that may be safely ignored in the shipped version */
#ifdef DEBUG
#define SOFTBREAK assert(false)
#define HARDBREAK assert(false) /* must never happed in the shipped version */
#else
#define SOFTBREAK
#define HARDBREAK UNREACHABLE
#endif

#define NOIMPL HARDBREAK

#define SVIEWARG(stringView) (i32)stringView.size(), stringView.data()

// TODO: doesn't C++ provide this define?
#define TOSTR(source) #source

#ifdef DEBUG
#define UNREACHABLE assert(false)
#endif

#ifdef _MSC_VER
#pragma warning(1:4062) /* The enumerate has no associated handler in a switch statement, and there is no default label. */

#include <intrin.h>

#define MSNZB32(tosearch, result) do { assert(unsigned long(tosearch) != 0); unsigned long r; _BitScanReverse(&r, unsigned long(tosearch)); *result = r; } while(0)
#define LSNZB32(tosearch, result) do { assert(unsigned long(tosearch) != 0); unsigned long r; _BitScanForward(&r, unsigned long(tosearch)); *result = r; } while(0)

#define MSNZB64(tosearch, result) do { assert(unsigned long long(tosearch) != 0); unsigned long r; _BitScanReverse64(&r, unsigned long long(tosearch)); *result = r; } while(0)
#define LSNZB64(tosearch, result) do { assert(unsigned long long(tosearch) != 0); unsigned long r; _BitScanForward64(&r, unsigned long long(tosearch)); *result = r; } while(0)

#ifndef UNREACHABLE
#define UNREACHABLE __assume(0)
#endif
#else
 // won't work with clang?
#pragma GCC diagnostic error "-Wswitch"

#define MSNZB32(tosearch, result) do { assert(unsigned int(tosearch) != 0); *result = (31 - __builtin_clz(unsigned int(tosearch))); } while(0)
#define LSNZB32(tosearch, result) do { assert(unsigned int(tosearch) != 0); *result = __builtin_ctz(unsigned int(tosearch)); } while(0)

#define MSNZB64(tosearch, result) do { assert(unsigned long long(tosearch) != 0); *result = (63 - __builtin_clzll(unsigned long long(tosearch))); } while(0)
#define LSNZB64(tosearch, result) do { assert(unsigned long long(tosearch) != 0); *result = __builtin_clzll(unsigned long long(tosearch)); } while(0)

#ifndef UNREACHABLE
#define UNREACHABLE __builtin_unreachable()
#endif
#endif

 /* The constant 0x5f37642f balances the relative error at +-0.034213.
 The constant 0x5f30c7f0 makes the relative error range from 0 to
 -0.061322.
 The constant 0x5f400000 makes the relative error range from 0 to
 +0.088662. */
inline f32 RSqrtVeryFast(f32 x0)
{
    union { i32 ix; f32 x; };

    x = x0;
    ix = 0x5f37642f - (ix >> 1); // Initial guess.
    return x;
}

 /* Notes: For more accuracy, repeat the Newton step (just duplicate the
 line). The routine always gets the result too low. According to Chris
 Lomont, the relative error is at most -0.00175228.
 Therefore, to cut its relative error in half, making it approximately
 plus or minus 0.000876, change the 1.5f in the Newton step to 1.500876f
 (1.5008908 works best for me, rel err is +-0.0008911).
 Chris says that changing the hex constant to 0x5f375a86 reduces the
 maximum relative error slightly, to 0.00175124. (I get 0.00175128. But
 the best I can do is use 5f375a82, which gives rel err = 0 to
 -0.00175123). However, using that value seems to usually give a slightly
 larger relative error, according to Chris.
 The routine can be adapted to IEEE double precision. */
inline f32 RSqrtFast(f32 x0)
{
    union { i32 ix; f32 x; };

    x = x0;
    f32 xhalf = 0.5f * x;
    ix = 0x5f375a82 - (ix >> 1);    // Initial guess.
    x = x * (1.5008908f - xhalf * x * x);   // Newton step.
    return x;
}

/* This is rsqrt with an additional step of the Newton iteration, for
increased accuracy. The constant 0x5f37599e makes the relative error
range from 0 to -0.00000463.
You can't balance the error by adjusting the constant. */
inline f32 RSqrtPrecise(f32 x)
{
    float xhalf = 0.5f*x;
    i32 i = *(i32 *)&x;
    i = 0x5f37599e - (i >> 1);   // Initial guess.
    x = *(f32 *)&i;            // View i as float.
    x = x * (1.5f - xhalf * x * x);    // Newton step.
    x = x * (1.5f - xhalf * x * x);    // Newton step again.
    return x;
}

template <typename T> inline void MemCpy(T *__restrict target, const T *source, uiw size)
{
	memcpy(target, source, size * sizeof(T));
}

template <typename T> inline T ResetBit(T value, uiw bitNumber)
{
    assert(bitNumber < sizeof(T) * 8);
    return value & ~(1 << bitNumber);
}

template <typename T> inline T SetBit(T value, uiw bitNumber)
{
    assert(bitNumber < sizeof(T) * 8);
    return value | (1 << bitNumber);
}

template <typename T> inline bool IsBitSet(T value, uiw bitNumber)
{
    assert(bitNumber < sizeof(T) * 8);
    return (value & (1 << bitNumber)) != 0;
}

constexpr ui32 RenderTargetsLimit = 8;

template <typename Tin> constexpr size_t CountOf()
{
    using T = typename std::remove_reference_t<Tin>;
    static_assert(std::is_array_v<T>, "CountOf() requires an array argument");
    static_assert(std::extent_v<T> > 0, "zero- or unknown-size array");
    return std::extent_v<T>;
}
#define CountOf(a) CountOf<decltype(a)>()
#define CountOf32(a) ((ui32)CountOf<decltype(a)>())