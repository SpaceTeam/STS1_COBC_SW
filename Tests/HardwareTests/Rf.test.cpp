#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>

#include <Tests/HardwareTests/Utility.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
using RODOS::PRINTF;


#if HW_VERSION >= 27
auto rfLatchupDisableGpioPin = hal::GpioPin(hal::rfLatchupDisablePin);
#endif


class RfTest : public RODOS::StaticThread<>
{
public:
    RfTest() : StaticThread("RfTest")
    {
    }


private:
    void init() override
    {
    }


    void run() override
    {
        PRINTF("\nRF test\n\n");

#if HW_VERSION >= 27
        rfLatchupDisableGpioPin.Reset();
#endif

        rf::Initialize(rf::TxType::morse);
        PRINTF("RF module initialized\n");

        PRINTF("\n");
        auto partNumber = rf::ReadPartNumber();
        PRINTF("Part number: 0x%4x == 0x%4x\n", partNumber, rf::correctPartNumber);
        Check(partNumber == rf::correctPartNumber);

        // Here comes the rest of the RF test
    }
} rfTest;
}
