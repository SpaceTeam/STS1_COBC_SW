#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <array>
#include <span>


namespace sts1cobcsw
{
using sts1cobcsw::operator""_b;


TEST_CASE("Littlefs CRC-32")
{
    auto byteData = std::to_array<Byte>({0xDE_b,
                                         0xAD_b,
                                         0xBE_b,
                                         0xEF_b,
                                         0xCA_b,
                                         0xBB_b,
                                         0xA5_b,
                                         0xE3_b,
                                         0xAB_b,
                                         0xFF_b,
                                         0x10_b,
                                         0x00_b,
                                         0x00_b,
                                         0x00_b,
                                         0x00_b});
    auto data1 = Span(byteData).first<8>();
    auto result = utility::ComputeCrc32(data1);
    CHECK(result == 0x1ffa30e0U);

    auto data2 = Span(byteData).first<9>();
    result = utility::ComputeCrc32(data2);
    CHECK(result == 0xe1116228U);

    auto data3 = Span(byteData).first<10>();
    result = utility::ComputeCrc32(data3);
    CHECK(result == 0x18565615U);

    auto data4 = Span(byteData).first<11>();
    result = utility::ComputeCrc32(data4);
    CHECK(result == 0x7072a2d9U);

    // Crc for the first 5 and then the next 6 elements is the same as with all 11
    // at a time
    auto data5 = Span(byteData).first<5>();
    auto data6 = Span(byteData).first<11>().subspan(5);
    result = utility::ComputeCrc32(data5);
    result = utility::ComputeCrc32(result, data6);
    CHECK(result == 0x7072a2d9U);

    byteData[11] = static_cast<Byte>(result);
    byteData[12] = static_cast<Byte>(result >> 8U);
    byteData[13] = static_cast<Byte>(result >> 16U);
    byteData[14] = static_cast<Byte>(result >> 24U);

    // Add crc in little edian and recompute -> crc == 0
    auto dataWithCrc = Span(byteData);
    result = utility::ComputeCrc32(dataWithCrc);
    CHECK(result == 0x00U);
}
}
