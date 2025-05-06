#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <etl/vector.h>

#include <algorithm>
#include <span>


using sts1cobcsw::ComputeCrc32;
using sts1cobcsw::Span;
using sts1cobcsw::operator""_b;


TEST_CASE("CRC-32")
{
    auto data = etl::vector<sts1cobcsw::Byte, 20>{
        0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b, 0xCA_b, 0xBB_b, 0xA5_b, 0xE3_b, 0xAB_b, 0xFF_b, 0x10_b};

    auto result = ComputeCrc32(Span(data).first<8>());
    CHECK(result == 0x1ffa30e0U);
    result = ComputeCrc32(Span(data).first<9>());
    CHECK(result == 0xe1116228U);
    result = ComputeCrc32(Span(data).first<10>());
    CHECK(result == 0x18565615U);
    result = ComputeCrc32(Span(data));
    CHECK(result == 0x7072a2d9U);

    // Splitting the CRC computation in two yields the same result as computing it in one go
    result = ComputeCrc32(Span(data).first<5>());
    result = ComputeCrc32(result, Span(data).subspan(5));
    CHECK(result == ComputeCrc32(Span(data)));

    auto serializedCrc = sts1cobcsw::Serialize(result);
    data.insert(data.end(), serializedCrc.begin(), serializedCrc.end());
    result = ComputeCrc32(Span(data));
    CHECK(result == 0x00U);
}
