#pragma once

#include <type_safe/types.hpp>

#include <etl/string.h>

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <ranges>
#include <span>


namespace sts1cobcsw
{
namespace ts = type_safe;


namespace util
{
auto CopyTo(std::span<std::byte> buffer, ts::size_t * const position, auto value)
{
    auto newPosition = *position + sizeof(value);
    std::memcpy(&buffer[(*position).get()], &value, sizeof(value));
    *position = newPosition;
}


template<std::size_t size>
auto CopyFrom(etl::string<size> const & buffer, ts::size_t * const position, auto * value)
{
    auto newPosition = *position + sizeof(*value);
    std::memcpy(value, &buffer[(*position).get()], sizeof(*value));
    *position = newPosition;
}


// std::bit_cast needs GCC 11 or later. This is the possible implementation from
// https://en.cppreference.com/w/cpp/numeric/bit_cast
template<class To, class From>
std::enable_if_t<sizeof(To) == sizeof(From)
                     && std::is_trivially_copyable_v<From> && std::is_trivially_copyable_v<To>,
                 To>
bit_cast(From const & src) noexcept  // NOLINT(readability-identifier-naming,
                                     // modernize-use-trailing-return-type)
{
    static_assert(
        std::is_trivially_constructible_v<To>,
        "This implementation additionally requires destination type to be trivially constructible");

    auto dst = To();
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}


// std::byteswap needs GCC 12 or later. This is the possible implementation from
// https://en.cppreference.com/w/cpp/numeric/byteswap
template<std::integral T>
constexpr auto byteswap(T value) noexcept  // NOLINT(readability-identifier-naming)
{
    static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
    auto valueRepresentation = util::bit_cast<std::array<std::byte, sizeof(T)>>(value);
    std::ranges::reverse(valueRepresentation);
    return util::bit_cast<T>(valueRepresentation);
}
}
}
