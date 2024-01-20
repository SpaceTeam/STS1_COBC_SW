//! @file
//! @brief  A program to test the STM32F411 CRC32 peripheral.
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
//!
//! @author Daniel Schloms <daniel.schloms@spaceteam.at>


#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <Tests/HardwareTests/Utility.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <cinttypes>
#include <span>


namespace sts1cobcsw
{
using RODOS::PRINTF;


auto const testByteData = std::to_array<Byte>(
    {0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b, 0xCA_b, 0xBB_b, 0xA5_b, 0xE3_b, 0xAB_b, 0xFF_b, 0x10_b});
auto const truthByteData =
    std::to_array<std::uint32_t>({0x78B4282BU, 0x3B485F9B, 0x75F77B52, 0x687DB322});

auto const testWordData = std::to_array<std::uint32_t>({0xDEADBEEF, 0xCABBA5E3});
std::uint32_t const truthWordData = 0xA962D97B;


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
        PRINTF("\nCRC32 test\n\n");

        utility::InitializeCrc32Hardware();
        static_assert(testByteData.size() > sizeof(std::uint32_t));
        auto nTrailingBytes = testByteData.size() % sizeof(std::uint32_t);
        RODOS::PRINTF(
            "INFO: Keep in mind that the DMA CRC32 version inverts endianness word wise!\n"
            "So {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xBB, 0xA5, 0xE3, 0xAB, 0xFF, 0x10}\n"
            " = {0xEF, 0xBE, 0xAD, 0xDE, 0xE3, 0xA5, 0xBB, 0xCA, 0x00, 0x10, 0xFF, 0xAB}\n"
            "when using a byte-based CRC32 calculator.\n");

        // Currently the DMA CRC32 is implemented in a blocking manner, so we can just test like
        // this
        for(std::size_t i = 0; i <= nTrailingBytes; ++i)
        {
            PRINTF("\n");
            PRINTF("Test DMA CRC32 with %d trailing bytes\n\n", static_cast<int>(i));
            PRINTF("Data:");
            auto data = Span(testByteData).first(testByteData.size() - nTrailingBytes + i);
            for(auto const & element : data)
            {
                PRINTF(" 0x%02x", static_cast<std::uint8_t>(element));
            }

            auto result = utility::ComputeCrc32(data);
            PRINTF("CRC32: 0x%08x == 0x%08x\n",
                   static_cast<unsigned int>(result),
                   static_cast<unsigned int>(truthByteData[i]));
            Check(result == truthByteData[i]);
        }

        PRINTF("\n");
        PRINTF("Test non-DMA HW CRC32 (words) -> keeps endianness\n");
        auto wordDataResult = utility::ComputeCrc32Blocking(Span(testWordData));
        PRINTF("CRC32: 0x%08x == 0x%08x\n",
               static_cast<unsigned int>(wordDataResult),
               static_cast<unsigned int>(truthWordData));
        Check(wordDataResult == truthWordData);
    }
} hardwareCrc32Test;
}
