#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>
#include <Tests/HardwareTests/Utility.hpp>

#include <Sts1CobcSw/Periphery/Rf.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
using RODOS::PRINTF;


class RfTest : public RODOS::StaticThread<>
{
public:
    RfTest() : StaticThread("RfTest")
    {
    }


private:
    void init() override
    {
        InitializeRfLatchupDisablePins();
    }


    void run() override
    {
        EnableRfLatchupProtection();

        PRINTF("\nRF test\n\n");

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
