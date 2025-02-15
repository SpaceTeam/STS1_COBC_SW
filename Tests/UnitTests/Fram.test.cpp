#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/RodosTime.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos/support/support-libs/random.h>
#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <array>
#include <cstdint>


namespace fram = sts1cobcsw::fram;

using sts1cobcsw::Byte;
using sts1cobcsw::ms;
using sts1cobcsw::Span;
using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)


TEST_CASE("FRAM")
{
    fram::Initialize();
    auto actualBaudRate = fram::ActualBaudRate();
    CHECK(actualBaudRate == 6'000'000);

    auto deviceId = fram::ReadDeviceId();
    CHECK(deviceId == fram::correctDeviceId);

    RODOS::setRandSeed(static_cast<std::uint64_t>(RODOS::NOW()));
    auto address = fram::Address(RODOS::uint32Rand() % (value_of(fram::memorySize)));
    static constexpr auto dataSize = 16 * 1024U;
    auto writtenData = std::array<Byte, dataSize>{};
    auto readData = std::array<Byte, dataSize>{};
    // The timeout doesn't matter because we don't run the SPI supervisor thread in this test
    fram::WriteTo(address, Span(writtenData), 30 * ms);
    fram::ReadFrom(address, Span(&readData), 30 * ms);
    CHECK(readData == writtenData);

    writtenData.fill(0xFF_b);
    fram::WriteTo(address, Span(writtenData), 30 * ms);
    fram::ReadFrom(address, Span(&readData), 30 * ms);
    CHECK(readData == writtenData);

    writtenData.fill(0x00_b);
    fram::WriteTo(address, Span(writtenData), 30 * ms);
    fram::ReadFrom(address, Span(&readData), 30 * ms);
    CHECK(readData == writtenData);
}
