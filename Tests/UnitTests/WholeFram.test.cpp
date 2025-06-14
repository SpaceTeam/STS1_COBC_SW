#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <utility>


namespace fram = sts1cobcsw::fram;

using sts1cobcsw::Byte;
using sts1cobcsw::ms;
using sts1cobcsw::Span;
using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)


TEST_CASE("Reading and writing the whole FRAM")
{
    fram::Initialize();
    auto actualBaudRate = fram::ActualBaudRate();
    CHECK(actualBaudRate == 6'000'000);

    auto deviceId = fram::ReadDeviceId();
    CHECK(deviceId == fram::correctDeviceId);

    static constexpr auto chunkSize = 16 * 1024U;
    static_assert(value_of(fram::memorySize) % chunkSize == 0);
    static constexpr auto endAddress = fram::Address(value_of(fram::memorySize));
    for(auto address = fram::Address(0); address < endAddress; address += fram::Size(chunkSize))
    {
        auto writtenData = std::array<Byte, chunkSize>{};
        auto readData = std::array<Byte, chunkSize>{};
        // The timeout doesn't matter because we don't run the SPI supervisor thread in this test
        fram::WriteTo(address, Span(writtenData), 30 * ms);
        fram::ReadFrom(address, Span(&readData), 30 * ms);
        CHECK(readData == writtenData);
        if(readData != writtenData)
        {
            RODOS::PRINTF("Address of the error: %lx\n",
                          static_cast<unsigned long>(value_of(address)));
        }

        writtenData.fill(0xFF_b);
        fram::WriteTo(address, Span(writtenData), 30 * ms);
        fram::ReadFrom(address, Span(&readData), 30 * ms);
        CHECK(readData == writtenData);
        if(readData != writtenData)
        {
            RODOS::PRINTF("Address of the error: %lx\n",
                          static_cast<unsigned long>(value_of(address)));
        }

        writtenData.fill(0x00_b);
        fram::WriteTo(address, Span(writtenData), 30 * ms);
        fram::ReadFrom(address, Span(&readData), 30 * ms);
        CHECK(readData == writtenData);
        if(readData != writtenData)
        {
            RODOS::PRINTF("Address of the error: %lx\n",
                          static_cast<unsigned long>(value_of(address)));
        }
    }
}
