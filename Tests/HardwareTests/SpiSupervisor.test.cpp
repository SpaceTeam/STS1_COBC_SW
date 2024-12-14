#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>

#include <Sts1CobcSw/CobcSoftware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/RodosTime.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos/support/support-libs/random.h>
#include <rodos_no_using_namespace.h>

#include <array>
#include <cstdint>


namespace sts1cobcsw
{
// Running this test showed that the minimum required stack size is between 12.0 and 12.1 kB
constexpr auto stackSize = 13'000;


class SpiSupervisorTest : public RODOS::StaticThread<stackSize>
{
public:
    SpiSupervisorTest() : StaticThread("SpiSupervisorTest")
    {
    }


private:
    void init() override
    {
        InitializeRfLatchupDisablePins();
    }


    void run() override
    {
        using RODOS::PRINTF;

        EnableRfLatchupProtection();
        SuspendFor(totalStartupTestTimeout);

        PRINTF("\nSPI supervisor test\n\n");

        PRINTF("\n");
        std::uint32_t const flashAddress = 0x00'01'10'00U;
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
