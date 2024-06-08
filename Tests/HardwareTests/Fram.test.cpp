#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>
#include <Tests/HardwareTests/Utility.hpp>

#include <rodos/support/support-libs/random.h>
#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw
{
using RODOS::PRINTF;
using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)


constexpr std::size_t testDataSize = 11 * 1024;  // 11 KiB
auto testData = std::array<Byte, testDataSize>{};
auto readData = std::array<Byte, testDataSize>{};
// Baud rate = 6 MHz, largest data transfer = 11 KiB -> spiTimeout = 30 ms is enough for all
// transfers
constexpr auto spiTimeout = 30 * RODOS::MILLISECONDS;


auto PrintDeviceId(fram::DeviceId const & deviceId) -> void;
auto WriteAndReadTestData(fram::Address const & address) -> void;


class FramTest : public RODOS::StaticThread<>
{
public:
    FramTest() : StaticThread("FramTest")
    {
    }


private:
    void init() override
    {
#if HW_VERSION >= 27
        rfLatchupDisableGpioPin.Direction(hal::PinDirection::out);
#endif
        fram::Initialize();
    }


    void run() override
    {
#if HW_VERSION >= 27
        rfLatchupDisableGpioPin.Reset();
#endif

        PRINTF("\nFRAM test\n\n");

        PRINTF("\n");
        auto actualBaudRate = fram::ActualBaudRate();
        PRINTF("Actual baud rate: %" PRIi32 "\n", actualBaudRate);

        PRINTF("\n");
        auto deviceId = fram::ReadDeviceId();
        PRINTF("Device ID: ");
        PrintDeviceId(deviceId);
        PRINTF(" ==\n");
        PRINTF("           ");
        PrintDeviceId(fram::correctDeviceId);
        Check(deviceId == fram::correctDeviceId);

        RODOS::setRandSeed(static_cast<std::uint64_t>(RODOS::NOW()));
        constexpr std::uint32_t nAdressBits = 20U;
        auto address = fram::Address{RODOS::uint32Rand() % (1U << nAdressBits)};

        PRINTF("\n");
        WriteAndReadTestData(address);
        std::fill(testData.begin(), testData.end(), 0xFF_b);
        WriteAndReadTestData(address);
        std::fill(testData.begin(), testData.end(), 0x00_b);
        WriteAndReadTestData(address);
    }
} framTest;


auto PrintDeviceId(fram::DeviceId const & deviceId) -> void
{
    PRINTF("0x");
    PRINTF("%02x", static_cast<unsigned int>(deviceId[8]));
    PRINTF("'");
    PRINTF("%02x", static_cast<unsigned int>(deviceId[7]));
    PRINTF("%02x", static_cast<unsigned int>(deviceId[6]));
    PRINTF("'");
    PRINTF("%02x", static_cast<unsigned int>(deviceId[5]));
    PRINTF("%02x", static_cast<unsigned int>(deviceId[4]));
    PRINTF("'");
    PRINTF("%02x", static_cast<unsigned int>(deviceId[3]));
    PRINTF("%02x", static_cast<unsigned int>(deviceId[2]));
    PRINTF("'");
    PRINTF("%02x", static_cast<unsigned int>(deviceId[1]));
    PRINTF("%02x", static_cast<unsigned int>(deviceId[0]));
}


auto WriteAndReadTestData(fram::Address const & address) -> void
{
    auto nBytesToPrint = 10U;

    PRINTF("\n");
    PRINTF("Writing %d bytes to address   0x%08x ...\n",
           static_cast<int>(testDataSize),
           static_cast<unsigned int>(address));
    auto begin = RODOS::NOW();
    fram::WriteTo(address, Span(testData), spiTimeout);
    auto end = RODOS::NOW();
    PRINTF("  took %d us\n", static_cast<int>((end - begin) / RODOS::MICROSECONDS));

    PRINTF("Reading %d bytes from address 0x%08x ...\n",
           static_cast<int>(testDataSize),
           static_cast<unsigned int>(address));
    begin = RODOS::NOW();
    fram::ReadFrom(address, Span(&readData), spiTimeout);
    end = RODOS::NOW();
    PRINTF("  took %d us\n", static_cast<int>((end - begin) / RODOS::MICROSECONDS));

    PRINTF("\n");
    PRINTF("Comparing first %d written and read bytes:\n", nBytesToPrint);
    PRINTF("  ");
    for(auto byte : Span(testData).first(nBytesToPrint))
    {
        PRINTF("0x%02x ", static_cast<unsigned char>(byte));
    }
    PRINTF("\n  ");
    for(auto byte : Span(readData).first(nBytesToPrint))
    {
        PRINTF("0x%02x ", static_cast<unsigned char>(byte));
    }
    PRINTF("\n");
    PRINTF("Comparing the full arrays ...\n");
    Check(readData == testData);
}
}
