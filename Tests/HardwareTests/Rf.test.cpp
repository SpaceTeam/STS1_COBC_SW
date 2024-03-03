#include <Sts1CobcSw/Periphery/Rf.hpp>

#include <Tests/HardwareTests/Utility.hpp>

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
    }


    void run() override
    {
        PRINTF("\nRF test\n\n");

        rf::Initialize(rf::TxType::morse);
        PRINTF("RF module initialized\n");

        PRINTF("\n");
        auto correctPartNumber = 0x4463;
        auto partNumber = rf::ReadPartNumber();
        PRINTF("Part number: 0x%4x == 0x%4x\n", partNumber, correctPartNumber);
        Check(partNumber == correctPartNumber);

        // Here comes the rest of the RF test
    }
} rfTest;
}
