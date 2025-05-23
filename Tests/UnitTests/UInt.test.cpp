#include <Sts1CobcSw/Serial/UInt.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>


using sts1cobcsw::AUInt;
using sts1cobcsw::SmallestUnsignedType;
using sts1cobcsw::UInt;


// This struct allows us to use std::is_invocable_v to test if constructing a UInt<> does not
// compile without actually throwing compiler errors. See Span.test.cpp for an explanation.
struct ConstructUInt
{
    template<std::size_t nBits>
    auto operator()(std::integral_constant<std::size_t, nBits>) const -> decltype(UInt<nBits>{});
};


// AUInt
static_assert(AUInt<UInt<0>>);
static_assert(AUInt<UInt<1>>);
static_assert(AUInt<UInt<5>>);
static_assert(AUInt<UInt<11>>);
static_assert(AUInt<UInt<17>>);
static_assert(AUInt<UInt<29>>);
static_assert(AUInt<UInt<37>>);
static_assert(AUInt<UInt<53>>);
static_assert(AUInt<UInt<64>>);
static_assert(not AUInt<int>);
static_assert(not AUInt<unsigned int>);
static_assert(not AUInt<double>);
static_assert(not AUInt<unsigned char>);

// Construction
static_assert(std::is_invocable_v<ConstructUInt, std::integral_constant<std::size_t, 0>>);
static_assert(std::is_invocable_v<ConstructUInt, std::integral_constant<std::size_t, 64>>);
static_assert(not std::is_invocable_v<ConstructUInt, std::integral_constant<std::size_t, 65>>);
static_assert(not std::is_invocable_v<ConstructUInt, std::integral_constant<std::size_t, 123>>);

// UnderlyingType
static_assert(std::is_same_v<UInt<0>::UnderlyingType, std::uint8_t>);
static_assert(std::is_same_v<UInt<1>::UnderlyingType, std::uint8_t>);
static_assert(std::is_same_v<UInt<8>::UnderlyingType, std::uint8_t>);
static_assert(std::is_same_v<UInt<9>::UnderlyingType, std::uint16_t>);
static_assert(std::is_same_v<UInt<16>::UnderlyingType, std::uint16_t>);
static_assert(std::is_same_v<UInt<17>::UnderlyingType, std::uint32_t>);
static_assert(std::is_same_v<UInt<32>::UnderlyingType, std::uint32_t>);
static_assert(std::is_same_v<UInt<33>::UnderlyingType, std::uint64_t>);
static_assert(std::is_same_v<UInt<64>::UnderlyingType, std::uint64_t>);

// size
static_assert(UInt<0>::size == 0);
static_assert(UInt<1>::size == 1);
static_assert(UInt<8>::size == 8);
static_assert(UInt<9>::size == 9);
static_assert(UInt<16>::size == 16);
static_assert(UInt<17>::size == 17);
static_assert(UInt<32>::size == 32);
static_assert(UInt<33>::size == 33);
static_assert(UInt<64>::size == 64);

// mask
static_assert(UInt<0>::mask == 0);
static_assert(UInt<1>::mask == 1);
static_assert(UInt<3>::mask == 0b111);
static_assert(UInt<8>::mask == 0xFF);
static_assert(UInt<9>::mask == 0x1FF);
static_assert(UInt<16>::mask == 0xFFFF);
static_assert(UInt<17>::mask == 0x1'FFFF);
static_assert(UInt<32>::mask == 0xFFFF'FFFF);
static_assert(UInt<33>::mask == 0x1'FFFF'FFFF);
static_assert(UInt<64>::mask == 0xFFFF'FFFF'FFFF'FFFF);

// ToUnderlying()
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Woverflow"
constexpr auto uint0 = UInt<0>(0xFF);
constexpr auto uint1 = UInt<1>(0xFF);
constexpr auto uint8 = UInt<8>(0xFFFF);
constexpr auto uint9 = UInt<9>(0xFFFF);
constexpr auto uint16 = UInt<16>(0xFF'FFFF);
constexpr auto uint17 = UInt<17>(0xFF'FFFF);
constexpr auto uint32 = UInt<32>(0xFF'FFFF'FFFF);
constexpr auto uint33 = UInt<33>(0xFF'FFFF'FFFF);
constexpr auto uint64 = UInt<64>(0xFFFF'FFFF'FFFF'FFFF);
#pragma GCC diagnostic pop

static_assert(uint0.ToUnderlying() == 0);
static_assert(uint1.ToUnderlying() == 1);
static_assert(uint8.ToUnderlying() == 0xFF);
static_assert(uint9.ToUnderlying() == 0x1FF);
static_assert(uint16.ToUnderlying() == 0xFFFF);
static_assert(uint17.ToUnderlying() == 0x1FFFF);
static_assert(uint32.ToUnderlying() == 0xFFFF'FFFF);
static_assert(uint33.ToUnderlying() == 0x1FFFF'FFFF);
static_assert(uint64.ToUnderlying() == 0xFFFF'FFFF'FFFF'FFFF);

// operator==()
static_assert(uint0 == UInt<0>{});
static_assert(uint0 == UInt<17>{});
static_assert(uint8 != uint33);
static_assert(uint16 == UInt<54>(0xFFFF));


TEST_CASE("UInt")  // NOLINT(cert-err58-cpp)
{
    CHECK(true);
}
