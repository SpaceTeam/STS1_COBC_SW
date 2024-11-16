#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Hal/SpiMock.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramMock.hpp>
#include <Sts1CobcSw/Periphery/Spis.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/RodosTime.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>

#include <rodos_no_using_namespace.h>

#include <cstddef>


namespace sts1cobcsw
{
using RODOS::PRINTF;


auto WriteThatFinishesInTime([[maybe_unused]] void const * data,
                             [[maybe_unused]] std::size_t nBytes,
                             Duration duration) -> void;
auto WriteThatNeverFinishes([[maybe_unused]] void const * data,
                            [[maybe_unused]] std::size_t nBytes,
                            Duration duration) -> void;


auto transferEnd = endOfTime;


class SpiSupervisorTest : public RODOS::StaticThread<>
{
    // We cannot use dynamic_cast because RTTI is disabled
    // NOLINTBEGIN(*static-cast-downcast)
    hal::SpiMock & flashSpi_ = static_cast<hal::SpiMock &>(flashSpi);
    hal::SpiMock & framEpsSpi_ = static_cast<hal::SpiMock &>(framEpsSpi);
    hal::SpiMock & rfSpi_ = static_cast<hal::SpiMock &>(rfSpi);
    // NOLINTEND(*static-cast-downcast)


public:
    SpiSupervisorTest() : StaticThread("SpiSupervisorTest", 200)
    {
    }

    void init() override
    {
        flashSpi_.SetTransferEnd([]() { return transferEnd; });
        flashSpi_.SetWrite(WriteThatFinishesInTime);
        framEpsSpi_.SetTransferEnd([]() { return transferEnd; });
        framEpsSpi_.SetWrite(WriteThatFinishesInTime);
        rfSpi_.SetTransferEnd([]() { return transferEnd; });
        rfSpi_.SetWrite(WriteThatFinishesInTime);
        fram::ram::SetAllDoFunctions();
        fram::Initialize();
    }

    void run() override
    {
        SuspendFor(1 * s);

        PRINTF("\nSPI supervisor test\n\n");

        PRINTF("Writing with implementation that finishes in time ...\n");
        WriteTo(&flashSpi_, Span(0x00_b), 10 * ms);
        WriteTo(&framEpsSpi_, Span(0x00_b), 100 * ms);
        // The supervision period is 1 s so this write will definitely be checked at least once by
        // the supervisor thread
        WriteTo(&rfSpi_, Span(0x00_b), 1500 * ms);
        PRINTF("  -> works\n");

        PRINTF("Writing with implementation that never finishes ...\n");
        flashSpi_.SetWrite(WriteThatNeverFinishes);
        WriteTo(&flashSpi_, Span(0x00_b), 1 * ms);
    }
} spiSupervisorTest;


auto WriteThatFinishesInTime([[maybe_unused]] void const * data,
                             [[maybe_unused]] std::size_t nBytes,
                             Duration duration) -> void
{
    transferEnd = CurrentRodosTime() + duration;
    SuspendFor(duration / 10 * 9);
    transferEnd = endOfTime;
}


auto WriteThatNeverFinishes([[maybe_unused]] void const * data,
                            [[maybe_unused]] std::size_t nBytes,
                            Duration duration) -> void
{
    transferEnd = CurrentRodosTime() + duration;
    SuspendUntil(endOfTime);
}
}
