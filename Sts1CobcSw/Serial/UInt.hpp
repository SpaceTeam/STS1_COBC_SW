#pragma once


#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>


namespace sts1cobcsw
{
// NOLINTBEGIN(*magic-numbers)
// clang-format off
template<std::size_t nBits>
    requires(nBits <= std::numeric_limits<std::uint64_t>::digits)
using SmallestUnsignedType =
    std::conditional_t<nBits <= 8, std::uint8_t,
    std::conditional_t<nBits <= 16, std::uint16_t,
    std::conditional_t<nBits <= 32, std::uint32_t, std::uint64_t>>>;
// clang-format on
// NOLINTEND(*magic-numbers)


// This class template and its member functions are so simple that I didn't add an .ipp file
template<std::size_t nBits>
    requires(nBits <= std::numeric_limits<std::uint64_t>::digits)
class UInt
{
public:
    using UnderlyingType = SmallestUnsignedType<nBits>;

    static constexpr auto size = nBits;
    static constexpr auto mask = nBits == std::numeric_limits<std::uint64_t>::digits
                                   ? std::numeric_limits<std::uint64_t>::max()
                                   : (1ULL << nBits) - 1ULL;

    constexpr UInt() = default;
    constexpr UInt(UnderlyingType value) : value_(value & mask)  // NOLINT(*explicit*)
    {
    }


    template<std::size_t rightNBits>
    friend constexpr auto operator==(UInt const & lhs, UInt<rightNBits> const & rhs) -> bool
    {
        return lhs.value_ == rhs.ToUnderlying();
    }


    [[nodiscard]] constexpr auto ToUnderlying() const -> UnderlyingType
    {
        return value_;
    }


private:
    UnderlyingType value_ = {};
};


template<typename T>
inline constexpr auto isAUInt = false;

template<std::size_t nBits>
inline constexpr auto isAUInt<UInt<nBits>> = true;


template<typename T>
concept AUInt = isAUInt<T>;
}
