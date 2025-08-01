#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
#include <Sts1CobcSw/Flash/Flash.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos/support/support-libs/random.h>
#include <rodos_no_using_namespace.h>

#include <array>
#include <cstdint>
#include <utility>


namespace sts1cobcsw
{
using RODOS::PRINTF;


namespace
{
// Running this test showed that the minimum required stack size is between 12.0 and 12.1 kB
constexpr auto stackSize = 13'000;


class SpiSupervisorTest : public RODOS::StaticThread<stackSize>
{
public:
    SpiSupervisorTest() : StaticThread("SpiSupervisorTest")
    {}


private:
    void init() override
    {}


    void run() override
    {
        SuspendFor(totalStartupTestTimeout);

        PRINTF("\nSPI supervisor test\n\n");

        PRINTF("\n");
        std::uint32_t const flashAddress = 0x0001'1000U;
        PRINTF("Reading flash page ...\n");
        auto page = flash::ReadPage(flashAddress);
        PRINTF("Writing flash page ...\n");
        flash::ProgramPage(flashAddress, Span(page));
        PRINTF("  done\n");

        PRINTF("\n");
        PRINTF("Suspending for 1 s ...\n");
        SuspendFor(1 * s);
        PRINTF("  done\n");

        PRINTF("\n");
        RODOS::setRandSeed(static_cast<std::uint64_t>(RODOS::NOW()));
        constexpr std::uint32_t nAdressBits = 20U;
        auto framAddress = fram::Address(RODOS::uint32Rand() % (1U << nAdressBits));

        auto framTestData = std::array<Byte, 11 * 1024>{};
        // Baud rate = 6 MHz, data size = 11 KiB -> transfer time ~ 15 ms
        constexpr auto spiTimeout = 30 * ms;
        PRINTF("Writing %d B to FRAM at address 0x%05x ...\n",
               static_cast<int>(framTestData.size()),
               static_cast<unsigned>(value_of(framAddress)));
        fram::WriteTo(framAddress, Span(framTestData), spiTimeout);
        PRINTF("  done\n\n");
    }
} spiSupervisorTest;
}
}
