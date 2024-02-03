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

        periphery::rf::Initialize();
        PRINTF("RF module initialized\n");

        PRINTF("\n");
        auto correctPartInfo = 0x4463;
        auto partInfo = periphery::rf::ReadPartInfo();
        PRINTF("Part info: 0x%4x == 0x%4x\n", partInfo, correctPartInfo);
        Check(partInfo == correctPartInfo);

        // Here comes the rest of the RF test
    }
} rfTest;
}
