#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Periphery/Fram.hpp>
#ifdef __linux__
    #include <Sts1CobcSw/Periphery/FramMock.hpp>
#endif
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

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
#ifdef __linux__
    fram::ram::SetAllDoFunctions();
    fram::ram::memory.fill(0x00_b);
#endif

    fram::Initialize();
    auto actualBaudRate = fram::ActualBaudRate();
    CHECK(actualBaudRate == 6'000'000);

    auto deviceId = fram::ReadDeviceId();
    CHECK(deviceId == fram::correctDeviceId);

    RODOS::setRandSeed(static_cast<std::uint64_t>(RODOS::NOW()));
    auto address = fram::Address(RODOS::uint32Rand() % (value_of(fram::memorySize)));

    // You read what you write
    {
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

#ifdef __linux__
    // Mocking the FRAM in RAM writes data as expected
    {
        auto writeData = std::array{0xAA_b, 0xBB_b, 0xCC_b, 0xDD_b};
        fram::WriteTo(address, Span(writeData), 0 * ms);
        CHECK(fram::ram::memory[value_of(address)] == writeData[0]);
        CHECK(fram::ram::memory[value_of(address) + 1] == writeData[1]);
        CHECK(fram::ram::memory[value_of(address) + 2] == writeData[2]);
        CHECK(fram::ram::memory[value_of(address) + 3] == writeData[3]);
    }
#endif
}


#ifdef __linux__
TEST_CASE("Empty FRAM mock functions do nothing by default")
{
    fram::empty::SetAllDoFunctions();
    fram::Initialize();
    auto deviceId = fram::ReadDeviceId();
    CHECK(deviceId == fram::DeviceId{});
    auto actualBaudRate = fram::ActualBaudRate();
    CHECK(actualBaudRate == 0);

    RODOS::setRandSeed(static_cast<std::uint64_t>(RODOS::NOW()));
    auto address = fram::Address(RODOS::uint32Rand() % (value_of(fram::memorySize)));

    auto readData = std::array{0x11_b, 0x22_b, 0x33_b, 0x44_b};
    fram::ReadFrom(address, Span(&readData), 0 * ms);
    CHECK(readData == (std::array{0x11_b, 0x22_b, 0x33_b, 0x44_b}));

    auto writeData = std::array{0xAA_b, 0xBB_b, 0xCC_b, 0xDD_b};
    fram::WriteTo(address, Span(writeData), 0 * ms);
    readData = fram::ReadFrom<writeData.size()>(address, 0 * ms);
    CHECK(readData == decltype(readData){});
}
#endif
