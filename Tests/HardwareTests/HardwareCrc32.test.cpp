//! @file HardwareCrc32.test.cpp
//! @author Daniel Schloms <daniel.schloms@spaceteam.at>
//! @brief A program to test the STM32F411 CRC32 peripheral.
//!
//! Preparation:
//!     - Connect the UCI UART to a computer to use with HTERM, Putty, etc.
//!     - Flash the program
//!     - Compare the results from the test data to some reference (e.g. https://crccalc.com/)
//!
//! Some reference results (MPEG-2):
//! DEADBEEFCABBA5E3:            0xA962D97B
//! DEADBEEFCABBA5E3000000AB:    0xBD5677DF
//! DEADBEEFCABBA5E30000ABFF:    0x8C49583B
//! DEADBEEFCABBA5E300ABFF10:    0xBE7357D8
//! EFBEADDEE3A5BBCA:            0x78B4282B

#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <rodos_no_using_namespace.h>

#include <array>

namespace sts1cobcsw
{
constexpr auto test0Trailing = std::to_array<Byte>(
    {0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b, 0xCA_b, 0xBB_b, 0xA5_b, 0xE3_b});  // word-aligned

constexpr auto test1Trailing = std::to_array<Byte>(
    {0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b, 0xCA_b, 0xBB_b, 0xA5_b, 0xE3_b, 0xAB_b});  // 1 trailing

constexpr auto test2Trailing = std::to_array<Byte>(
    {0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b, 0xCA_b, 0xBB_b, 0xA5_b, 0xE3_b, 0xAB_b, 0xFF_b});  // 2
                                                                                        // trailing
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
        utility::InitializeCrc32Hardware(utility::DmaBurstType::singleWord);

        // Currently the DMA CRC32 is implemented in a blocking manner, so we can just test like
        // NOLINTBEGIN(clang-diagnostic-format) 
        RODOS::PRINTF("DMA CRC32 0 Trailing:\n");
        RODOS::PRINTF("CRC32: %lu\n", utility::ComputeCrc32(Span(test0Trailing)));

        RODOS::PRINTF("DMA CRC32 1 Trailing:\n");
        RODOS::PRINTF("CRC32: %lu\n", utility::ComputeCrc32(Span(test1Trailing)));

        RODOS::PRINTF("DMA CRC32 2 Trailing:\n");
        RODOS::PRINTF("CRC32: %lu\n", utility::ComputeCrc32(Span(test2Trailing)));

        RODOS::PRINTF("DMA CRC32 3 Trailing:\n");
        RODOS::PRINTF("CRC32: %lu\n", utility::ComputeCrc32(Span(test3Trailing)));

        RODOS::PRINTF("Non-DMA HW CRC32 (words):\n");
        RODOS::PRINTF("CRC32: %lu\n", utility::ComputeCrc32Blocking(Span(&testWords)));
        // NOLINTEND(clang-diagnostic-format)
    }
} hardwareCrc32Test;
}
