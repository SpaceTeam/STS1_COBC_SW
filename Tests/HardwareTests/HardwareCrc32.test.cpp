//! @file HardwareCrc32.test.cpp
//! @author Daniel Schloms <daniel.schloms@spaceteam.at>
//! @brief A program to test the STM32F411 CRC32 peripheral.
//!
//! Preparation:
//!     - Connect the UCI UART to a computer to use with HTERM, Putty, etc.
//!     - Flash the program
//!     - Compare the results from the test data to some reference (e.g. https://crccalc.com/)
//!
//! Note that the DMA version inverts endianness word-wise!
//!
//! Some reference results (MPEG-2):
//! DEADBEEFCABBA5E3:           0xA962D97B
//! DEADBEEFCABBA5E3000000AB:   0xBD5677DF
//! DEADBEEFCABBA5E30000ABFF:   0x8C49583B
//! DEADBEEFCABBA5E300ABFF10:   0xBE7357D8
//! Test result references:
//! EFBEADDEE3A5BBCA:           0x78B4282B
//! EFBEADDEE3A5BBCA000000AB:   0x3B485F9B
//! EFBEADDEE3A5BBCA0000FFAB:   0x75F77B52
//! EFBEADDEE3A5BBCA0010FFAB:   0x687DB322

#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <span>

namespace sts1cobcsw
{
constexpr auto testByteData = std::to_array<Byte>(
    {0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b, 0xCA_b, 0xBB_b, 0xA5_b, 0xE3_b, 0xAB_b, 0xFF_b, 0x10_b});
constexpr auto truthByteData =
    std::to_array<uint32_t>({0x78B4282BU, 0x3B485F9B, 0x75F77B52, 0x687DB322});

auto testWordData = std::to_array<std::uint32_t>({0xDEADBEEF, 0xCABBA5E3});
constexpr auto truthWordData = static_cast<uint32_t>(0xA962D97BU);

class HardwareCrc32Test : public RODOS::StaticThread<>
{
public:
    HardwareCrc32Test() : StaticThread("HardwareCrc32Test")
    {
    }


private:
    void init() override
    {
    }

    void run() override
    {
        utility::InitializeCrc32Hardware();
        static_assert(testByteData.size() > sizeof(uint32_t));
        auto nTrailingBytes = testByteData.size() % sizeof(uint32_t);
        RODOS::PRINTF(
            "INFO: Keep in mind that the DMA CRC32 version inverts endianness word-wise!\nSo "
            "{0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xBB, 0xA5, 0xE3, 0xAB, 0xFF, 0x10}\n = {0xEF, 0xBE, "
            "0xAD, 0xDE, 0xE3, 0xA5, 0xBB, 0xCA, 0x00, 0x10, 0xFF, 0xAB}\nwhen using a byte-based "
            "CRC32 calculator.\n");

        // Currently the DMA CRC32 is implemented in a blocking manner, so we can just test like
        // this
        for(size_t i = 0; i <= nTrailingBytes; i++)
        {
            // NOLINTNEXTLINE(clang-diagnostic-format)
            RODOS::PRINTF("\nDMA CRC32 %u Trailing:\nData buffer:\n", i);

            auto dataSpan = Span(testByteData).subspan(0, testByteData.size() - nTrailingBytes + i);
            for(auto && element : dataSpan)
            {
                RODOS::PRINTF("%x ", static_cast<uint8_t>(element));
            }

            auto result = utility::ComputeCrc32(dataSpan);
            // NOLINTBEGIN(clang-diagnostic-format)
            RODOS::PRINTF("\nCRC32: %lx, Reference: %lx, %s\n",
                          result,
                          truthByteData[i],
                          result == truthByteData[i] ? "PASS" : "FAIL");
            // NOLINTEND(clang-diagnostic-format)
        }

        RODOS::PRINTF("\nNon-DMA HW CRC32 (words) -> keeps endianness:\n");
        auto wordDataResult = utility::ComputeCrc32Blocking(Span(&testWordData));
        // NOLINTBEGIN(clang-diagnostic-format)
        RODOS::PRINTF("CRC32: %lx, Reference: %lx, %s\n",
                      wordDataResult,
                      truthWordData,
                      wordDataResult == truthWordData ? "PASS" : "FAIL");
        // NOLINTEND(clang-diagnostic-format)
    }
} hardwareCrc32Test;
}
