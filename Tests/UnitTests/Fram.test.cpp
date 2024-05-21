#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramMock.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>

#include <array>
#include <string>


namespace fram = sts1cobcsw::fram;

using sts1cobcsw::Byte;
using sts1cobcsw::Span;
using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)


auto WriteAndReadTestData(sts1cobcsw::fram::Address const & address) -> void;
auto ReadCorrectDeviceId() -> fram::DeviceId;


TEST_CASE("Mocked functions do nothing by default")
{
    SECTION("Initialize(), ReadDeviceId() and ActualBaudRate()")
    {
        fram::Initialize();
        auto deviceId = fram::ReadDeviceId();
        CHECK(deviceId == fram::DeviceId{});
        auto actualBaudRate = fram::ActualBaudRate();
        CHECK(actualBaudRate == 0);
    }

    auto address = GENERATE(take(10, random(0U, 1U << 20U)));

    SECTION("WriteTo() and ReadFrom()")
    {
        auto readData = std::array{0x11_b, 0x22_b, 0x33_b, 0x44_b};
        fram::ReadFrom(address, Span(&readData), 0);
        CHECK(readData == std::array{0x11_b, 0x22_b, 0x33_b, 0x44_b});

        auto writeData = std::array{0xAA_b, 0xBB_b, 0xCC_b, 0xDD_b};
        fram::WriteTo(address, Span(writeData), 0);
        readData = fram::ReadFrom<writeData.size()>(address, 0);
        CHECK(readData == decltype(readData){});
    }
}


TEST_CASE("Mocking FRAM in RAM")
{
    fram::ram::SetAllDoFunctions();
    fram::ram::storage.fill(0x00_b);

    fram::Initialize();
    auto deviceId = fram::ReadDeviceId();
    CHECK(deviceId == fram::correctDeviceId);
    auto actualBaudRate = fram::ActualBaudRate();
    CHECK(actualBaudRate == 6'000'000);

    auto address = GENERATE(take(1, random(0U, fram::ram::storageSize - 10)));

    auto readData = std::array{0x01_b, 0x02_b, 0x03_b, 0x04_b};
    fram::ReadFrom(address, Span(&readData), 0);
    CHECK(readData == decltype(readData){});

    auto writeData = std::array{0xAA_b, 0xBB_b, 0xCC_b, 0xDD_b};
    fram::WriteTo(address, Span(writeData), 0);
    CHECK(fram::ram::storage[address] == writeData[0]);
    CHECK(fram::ram::storage[address + 1] == writeData[1]);
    CHECK(fram::ram::storage[address + 2] == writeData[2]);
    CHECK(fram::ram::storage[address + 3] == writeData[3]);

    readData = fram::ReadFrom<writeData.size()>(address, 0);
    CHECK(readData == writeData);
}
