#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>

#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <rodos/support/support-libs/random.h>
#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{


auto delay = 1 * RODOS::SECONDS;

class SpiSupervisorTest : public RODOS::StaticThread<>
{
public:
    SpiSupervisorTest() : StaticThread("SpiSupervisorTest")
    {
    }


private:
    void init() override
    {
#if HW_VERSION >= 27
        rfLatchupDisableGpioPin.Direction(hal::PinDirection::out);
#endif
    }


    void run() override
    {
        using RODOS::PRINTF;

#if HW_VERSION >= 27
        rfLatchupDisableGpioPin.Reset();
#endif

        std::uint32_t const flashAddress = 0x00'01'00'00U;

        RODOS::setRandSeed(static_cast<std::uint64_t>(RODOS::NOW()));
        constexpr std::uint32_t nAdressBits = 20U;
        auto framAddress = fram::Address{RODOS::uint32Rand() % (1U << nAdressBits)};

        constexpr std::size_t testDataSize = 11 * 1024;
        auto framTestData = std::array<Byte, testDataSize>{};
        // Baud rate = 6 MHz, largest data transfer = 11 KiB -> spiTimeout = 30 ms is enough for all
        // transfers
        constexpr auto spiTimeout = 30 * RODOS::MILLISECONDS;

        RODOS::AT(delay);

        PRINTF("Testing Flash writing\n");
        auto page = flash::ReadPage(flashAddress);
        flash::ProgramPage(flashAddress, Span(page));

        PRINTF("Testing Fram writing and reading\n");
        fram::WriteTo(framAddress, Span(framTestData), spiTimeout);
    }

} spiSupervisorTest;
}
