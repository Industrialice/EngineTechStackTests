#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define WINPLATFORM
#endif

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
#include <functional>
#include <variant>
#include <mutex>
#include <atomic>
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
using namespace std::literals::string_literals;

#include <StdPlatformAbstractionLib.hpp>
#include <MatrixMathTypes.hpp>
using namespace StdLib;

using string_view_utf8 = std::basic_string_view<utf8char>;
using string_view_utf32 = std::basic_string_view<utf32char>;
using string_utf8 = std::basic_string<utf8char>;
using string_utf32 = std::basic_string<utf32char>;

#define SVIEWARG(stringView) (i32)stringView.size(), stringView.data()

constexpr ui32 RenderTargetsLimit = 8;

template <typename T, typename E, E T::*field, typename IT> [[nodiscard]] E FindSmallestID(IT start, IT end)
{
    static_assert(std::is_unsigned_v<E>);
    
    if (start == end)
    {
        return 0;
    }

    vector<E> temp;
    for (; start != end; ++start)
    {
        const auto &value = *start;
        temp.push_back(value.*field);
    }

    if (temp.front() != 0)
    {
        return 0;
    }

    for (uiw index = 0, size = temp.size() - 1; index != size; ++index)
    {
        E nextId = temp[index] + 1;
        if (nextId != temp[index + 1])
        {
            return nextId;
        }
    }

    return temp.back() + 1;
}

template <typename T, typename E, E T::*field, typename IT>[[nodiscard]] NOINLINE E AssignId(E &currentId, IT start, IT end)
{
    if (currentId != std::numeric_limits<E>::max())
    {
        E id = currentId++;
        return id;
    }
    // this should never happen unless you have bogus code that calls AssignId repeatedly
    // you'd need 50k AssignId calls per second to exhaust ui32 within 24 hours
    SOFTBREAK;
    return FindSmallestID<T, E, field>(start, end);
}