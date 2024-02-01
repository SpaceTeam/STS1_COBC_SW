#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>

#include <Tests/HardwareTests/Utility.hpp>

#include <rodos_no_using_namespace.h>

#include <span>
#include <string_view>


namespace sts1cobcsw
{
using RODOS::PRINTF;


auto uciUart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);


class RfTest : public RODOS::StaticThread<>
{
public:
    RfTest() : StaticThread("RfTest")
    {
    }


private:
    void init() override
    {
        constexpr auto uartBaudRate = 115200;
        uciUart.init(uartBaudRate);
    }


    void run() override
    {
        PRINTF("\nRF test\n\n");

        periphery::rf::Initialize();
        PRINTF("RF module initialized\n");

        PRINTF("\n");
        auto correctPartInfo = 0x4463;
        auto partInfo = periphery::rf::GetPartInfo();
        PRINTF("Part info: 0x%4x == 0x%4x\n", partInfo, correctPartInfo);
        Check(partInfo == correctPartInfo);

        // Here comes the rest of the RF test
    }
} rfTest;
}
