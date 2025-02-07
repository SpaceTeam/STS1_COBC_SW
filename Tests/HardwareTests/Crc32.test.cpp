#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Crc32Hardware.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <array>
#include <cstdint>
#include <span>


namespace sts1cobcsw
{
using sts1cobcsw::operator""_b;


TEST_INIT("Initialize hardware CRC unit")
{
    utility::InitializeCrc32Hardware();
}


TEST_CASE("Hardware CRC-32 computation")
{
    // With DMA
    static constexpr auto byteData = std::to_array<Byte>(
        {0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b, 0xCA_b, 0xBB_b, 0xA5_b, 0xE3_b, 0xAB_b, 0xFF_b, 0x10_b});
    auto data1 = Span(byteData).first<8>();
    auto result = utility::ComputeCrc32(data1);
    CHECK(result == 0x78B4282BU);

    auto data2 = Span(byteData).first<9>();
    result = utility::ComputeCrc32(data2);
    CHECK(result == 0x3B485F9BU);

    auto data3 = Span(byteData).first<10>();
    result = utility::ComputeCrc32(data3);
    CHECK(result == 0x75F77B52U);

    auto data4 = Span(byteData).first<11>();
    result = utility::ComputeCrc32(data4);
    CHECK(result == 0x687DB322U);

    // Without DMA
    static constexpr auto wordData = std::to_array<std::uint32_t>({0xDEADBEEF, 0xCABBA5E3});
    result = utility::ComputeCrc32Blocking(Span(wordData));
    CHECK(result == 0xA962D97BU);
}
}
