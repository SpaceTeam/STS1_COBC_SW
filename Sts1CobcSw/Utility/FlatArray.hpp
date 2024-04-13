#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <type_traits>


namespace sts1cobcsw
{
template<typename T>
inline constexpr std::size_t flatArraySize = 1;

template<typename T, std::size_t size>
inline constexpr std::size_t flatArraySize<std::array<T, size>> = size;

template<typename T, std::size_t size>
inline constexpr std::size_t flatArraySize<const std::array<T, size>> = size;

template<typename T, std::size_t extent>
    requires(extent != std::dynamic_extent)
inline constexpr std::size_t flatArraySize<std::span<T, extent>> = extent;

template<typename T, std::size_t extent>
    requires(extent != std::dynamic_extent)
inline constexpr std::size_t flatArraySize<std::span<T const, extent>> = extent;

// Not sure if we need the following two overloads as well, but better safe than sorry
template<typename T, std::size_t extent>
    requires(extent != std::dynamic_extent)
inline constexpr std::size_t flatArraySize<const std::span<T, extent>> = extent;

template<typename T, std::size_t extent>
    requires(extent != std::dynamic_extent)
inline constexpr std::size_t flatArraySize<const std::span<T const, extent>> = extent;


//! @brief  Create a flat `std::array` from the given arrays, spans, and values.
//!
//! All arguments must be of the same type `T`, `std::array<T, size>`, or `std::span<T, extent>`.
//! The sizes/extents of the arrays/spans can differ, but the spans must not have dynamic extent.
//! The order of the elements in the resulting array is the same as the order of the arguments.
//!
//! The return type `std::array<T, size>` is not explicitly specified because computing `T` and
//! `size` from the arguments is not trivial.
template<typename... Args>
constexpr auto FlatArray(Args const &... args)
{
    // NOLINTNEXTLINE(readability-identifier-naming)
    auto Data = []<typename T>(T const & arrayOrValue)
    {
        // TODO: Use .data() instead of std::data()
        constexpr bool hasData = requires(T t) { std::data(t); };
        if constexpr(hasData)
        {
            return std::data(arrayOrValue);
        }
        else
        {
            return &arrayOrValue;
        }
    };
    using T = std::remove_cvref_t<decltype(*(Data(args), ...))>;
    // TODO: Turn this into a constraint if possible because than we can test it in the same way as
    // Span()
    static_assert((std::same_as<T, std::remove_cvref_t<decltype(*(Data(args)))>> && ...),
                  "All arguments of FlatArray() must all be of type T, array-of-T, or span-of-T");

    auto result = std::array<T, (0U + ... + flatArraySize<Args>)>{};
    if constexpr(result.size() != 0)
    {
        auto iterator = result.begin();
        ((iterator = std::copy_n(Data(args), flatArraySize<Args>, iterator)), ...);
    }
    return result;
}
}
