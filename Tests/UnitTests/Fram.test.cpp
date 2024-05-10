#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramMock.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <random>
#include <span>


namespace fram = sts1cobcsw::fram;


using sts1cobcsw::Byte;
using sts1cobcsw::Span;
using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)


constexpr auto spiTimeout = 1;  // in ms
constexpr auto nAddressBits = 20U;
size_t const testDataSize = 11 * 1024;  // 11 KiB
auto testData = std::array<Byte, testDataSize>{};
auto readData = std::array<Byte, testDataSize>{};


auto WriteAndReadTestData(sts1cobcsw::fram::Address const & address) -> void;
auto ReadCorrectDeviceId() -> fram::DeviceId;


TEST_CASE("Fram mock using ram")
{
    fram::FramMockMode(fram::MockMode::ram);

    std::mt19937 randomEngine(std::random_device{}());
    std::uniform_int_distribution<uint32_t> gen(0, (1U << nAddressBits) - 1);
    fram::Address const address = gen(randomEngine);

    WriteAndReadTestData(address);
    std::generate(
        testData.begin(), testData.end(), [&]() { return static_cast<std::byte>(randomEngine()); });
    WriteAndReadTestData(address);

    auto deviceId = fram::ReadDeviceId();
    constexpr auto correctDeviceId =
        std::to_array({0x03_b, 0x2E_b, 0xC2_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b});
    REQUIRE(deviceId != correctDeviceId);
    fram::SetDoReadDeviceId(ReadCorrectDeviceId);
    deviceId = fram::ReadDeviceId();
    REQUIRE(deviceId == correctDeviceId);
}


TEST_CASE("Fram mock using file")
{
    fram::FramMockMode(fram::MockMode::file);

    std::mt19937 randomEngine(std::random_device{}());
    std::uniform_int_distribution<std::uint32_t> addressGenerator(0, ((1U << nAddressBits) - 1));
    fram::Address const address = addressGenerator(randomEngine);

    WriteAndReadTestData(address);
    std::generate(
        testData.begin(), testData.end(), [&]() { return static_cast<std::byte>(randomEngine()); });
    WriteAndReadTestData(address);
}


auto WriteAndReadTestData(fram::Address const & address) -> void
{
    auto nBytesToPrint = 10U;

    std::printf("Writing %d bytes to address   0x%08x ...\n",
                static_cast<int>(testDataSize),
                static_cast<unsigned int>(address));
    fram::WriteTo(address, Span(testData), spiTimeout);

    std::printf("Reading %d bytes from address 0x%08x ...\n",
                static_cast<int>(testDataSize),
                static_cast<unsigned int>(address));
    fram::ReadFrom(address, Span(&readData), spiTimeout);

    std::printf("Comparing first %d written and read bytes:\n", nBytesToPrint);
    std::printf("  ");
    for(auto byte : Span(testData).first(nBytesToPrint))
    {
        std::printf("0x%02x ", static_cast<unsigned char>(byte));
    }
    std::printf("\n  ");
    for(auto byte : Span(readData).first(nBytesToPrint))
    {
        std::printf("0x%02x ", static_cast<unsigned char>(byte));
    }
    std::printf("\n");

    REQUIRE(readData == testData);
}


auto ReadCorrectDeviceId() -> fram::DeviceId
{
    return std::to_array({0x03_b, 0x2E_b, 0xC2_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b});
}
