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

namespace sts1cobcsw
{
constexpr auto test0Trailing = std::to_array<Byte>(
    {0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b, 0xCA_b, 0xBB_b, 0xA5_b, 0xE3_b});  // word-aligned
constexpr auto truth0Trailing = 0x78B4282BU;

constexpr auto test1Trailing = std::to_array<Byte>(
    {0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b, 0xCA_b, 0xBB_b, 0xA5_b, 0xE3_b, 0xAB_b});  // 1 trailing
constexpr auto truth1Trailing = 0x3B485F9B;

constexpr auto test2Trailing = std::to_array<Byte>(
    {0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b, 0xCA_b, 0xBB_b, 0xA5_b, 0xE3_b, 0xAB_b, 0xFF_b});  // 2
                                                                                        // trailing
constexpr auto truth2Trailing = 0x75F77B52;

constexpr auto test3Trailing = std::to_array<Byte>({0xDE_b,
                                                    0xAD_b,
                                                    0xBE_b,
                                                    0xEF_b,
                                                    0xCA_b,
                                                    0xBB_b,
                                                    0xA5_b,
                                                    0xE3_b,
                                                    0xAB_b,
                                                    0xFF_b,
                                                    0x10_b});  // 3 trailing
constexpr auto truth3Trailing = 0x687DB322;

auto testWords = std::to_array<std::uint32_t>({0xDEADBEEF, 0xCABBA5E3});

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

        // Currently the DMA CRC32 is implemented in a blocking manner, so we can just test like
        // NOLINTBEGIN(clang-diagnostic-format)
        RODOS::PRINTF("\nDMA CRC32 0 Trailing:\n");
        auto result0Trailing = utility::ComputeCrc32(Span(test0Trailing));
        RODOS::PRINTF("CRC32: %lx, Reference: %x, %s\n",
                      result0Trailing,
                      truth0Trailing,
                      result0Trailing == truth0Trailing ? "PASS" : "FAIL");

        RODOS::PRINTF("\nDMA CRC32 1 Trailing:\n");
        auto result1Trailing = utility::ComputeCrc32(Span(test1Trailing));
        RODOS::PRINTF("CRC32: %lx, Reference: %x, %s\n",
                      result1Trailing,
                      truth1Trailing,
                      result1Trailing == truth1Trailing ? "PASS" : "FAIL");

        RODOS::PRINTF("\nDMA CRC32 2 Trailing:\n");
        auto result2Trailing = utility::ComputeCrc32(Span(test2Trailing));
        RODOS::PRINTF("CRC32: %lx, Reference: %x, %s\n",
                      result2Trailing,
                      truth2Trailing,
                      result2Trailing == truth2Trailing ? "PASS" : "FAIL");

        RODOS::PRINTF("\nDMA CRC32 3 Trailing:\n");
        auto result3Trailing = utility::ComputeCrc32(Span(test3Trailing));
        RODOS::PRINTF("CRC32: %lx, Reference: %x, %s\n",
                      result3Trailing,
                      truth3Trailing,
                      result3Trailing == truth3Trailing ? "PASS" : "FAIL");

        RODOS::PRINTF("\nNon-DMA HW CRC32 (words) -> keeps endianness:\n");
        RODOS::PRINTF("CRC32: %lx\n", utility::ComputeCrc32Blocking(Span(&testWords)));
        // NOLINTEND(clang-diagnostic-format)
    }
} hardwareCrc32Test;
}
