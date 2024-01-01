#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <Tests/HardwareTests/Utility.hpp>

#include <rodos/support/support-libs/random.h>
#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <cinttypes>
#include <cstdint>


namespace sts1cobcsw
{
using RODOS::PRINTF;
using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)


const size_t testDataSize = 11 * 1024;  // 11 KiB


auto PrintDeviceId(fram::DeviceId const & deviceId) -> void;
auto WriteAndReadData(const fram::Address & address,
                      std::array<Byte, testDataSize> & data,
                      size_t nPrintedBytes) -> void;


class FramTest : public RODOS::StaticThread<>
{
public:
    FramTest() : StaticThread("FramTest")
    {
    }

private:
    void init() override
    {
        fram::Initialize();
    }

    void run() override
    {
        PRINTF("\nFRAM test\n\n");

        PRINTF("\n");
        auto actualBaudRate = fram::ActualBaudRate();
        PRINTF("Actual baud rate: %" PRIi32 "\n", actualBaudRate);

        PRINTF("\n");
        auto deviceId = fram::ReadDeviceId();
        PRINTF("Device ID: ");
        PrintDeviceId(deviceId);
        PRINTF(" ==\n");
        PRINTF("           0x7F'7F7F'7F7F'7FC2'2E03\n");
        Check(deviceId[8] == 0x7F_b);
        Check(deviceId[7] == 0x7F_b);
        Check(deviceId[6] == 0x7F_b);
        Check(deviceId[5] == 0x7F_b);
        Check(deviceId[4] == 0x7F_b);
        Check(deviceId[3] == 0x7F_b);
        Check(deviceId[2] == 0xC2_b);
        Check(deviceId[1] == 0x2E_b);
        Check(deviceId[0] == 0x03_b);

        PRINTF("\n");
        RODOS::setRandSeed(static_cast<std::uint64_t>(RODOS::NOW()));
        constexpr uint32_t nAdressBits = 20U;
        auto address = fram::Address{RODOS::uint32Rand() % (1U << nAdressBits)};

        auto testData = std::array<Byte, testDataSize>{};
        WriteAndReadData(address, testData, 10);
        std::fill(testData.begin(), testData.end(), 0xFF_b);
        WriteAndReadData(address, testData, 10);
        std::fill(testData.begin(), testData.end(), 0x00_b);
        WriteAndReadData(address, testData, 10);
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

auto WriteAndReadData(const fram::Address & address,
                      std::array<Byte, testDataSize> & data,
                      size_t nPrintedBytes) -> void
{
    fram::WriteTo(address, Span(data));
    PRINTF("Writing %zu bytes to   address 0x%08x:\n",
           nPrintedBytes,
           static_cast<unsigned int>(address));
    for(size_t i = 0; i < nPrintedBytes; i++)
    {
        PRINTF("0x%02x  ", static_cast<unsigned char>(data[i]));
    }
    auto readData = fram::ReadFrom<testDataSize>(address);
    PRINTF("Reading %zu bytes from address 0x%08x:\n",
           nPrintedBytes,
           static_cast<unsigned int>(address));
    for(size_t i = 0; i < nPrintedBytes; i++)
    {
        PRINTF("0x%02x  ", static_cast<unsigned char>(readData[i]));
    }
    Check(readData == data);
}
}
