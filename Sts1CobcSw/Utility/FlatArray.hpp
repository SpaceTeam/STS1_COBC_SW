#include <algorithm>
#include <array>
#include <cstddef>
#include <type_traits>


namespace sts1cobcsw
{
template<typename T>
inline constexpr std::size_t flatArraySize = 1;

template<typename T, std::size_t size>
inline constexpr std::size_t flatArraySize<std::array<T, size>> = size;

template<typename T, std::size_t size>
inline constexpr std::size_t flatArraySize<const std::array<T, size>> = size;


//! @brief  Create a flat `std::array` from the given arrays and/or values.
//!
//! All arguments must be of the same type `T` or `std::array<T, size>`, although the sizes of the
//! arrays can differ. The order of the elements in the resulting array is the same as the order of
//! the arguments.
//!
//! The return type `std::array<T, size>` is not explicitly specified because computing `T` and
//! `size` from the arguments is not trivial.
template<typename... Args>
constexpr auto FlatArray(Args const &... args)
{
    // NOLINTNEXTLINE(readability-identifier-naming)
    auto Data = []<typename T>(T const & arrayOrValue)
    {
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
    static_assert((std::same_as<T, std::remove_cvref_t<decltype(*(Data(args)))>> && ...),
                  "All arguments of FlatArray() must all be of type T or array-of-T");

    auto result = std::array<T, (0U + ... + flatArraySize<Args>)>{};
    if constexpr(result.size() != 0)
    {
        auto iterator = result.begin();
        ((iterator = std::copy_n(Data(args), flatArraySize<Args>, iterator)), ...);
    }
    return result;
}
}
