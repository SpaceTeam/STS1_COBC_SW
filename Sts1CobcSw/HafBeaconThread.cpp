#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/Periphery/RfNames.hpp>

#include <rodos_no_using_namespace.h>

#include <string_view>


namespace sts1cobcsw
{
constexpr auto callSign = periphery::rf::portableCallSign;
constexpr auto beaconStartTime = 5 * RODOS::SECONDS;
constexpr auto beaconPeriod = 30 * RODOS::SECONDS;


class HafBeaconThread : public RODOS::StaticThread<>
{
public:
    HafBeaconThread() : StaticThread("HafBeaconThread")
    {
    }


private:
    void init() override
    {
    }


    void run() override
    {
        TIME_LOOP(beaconStartTime, beaconPeriod)
        {
            RODOS::PRINTF("\n");
            RODOS::PRINTF("Morsing call sign ...\n");
            // Setting the TX type again is necessary because the result is transmitted in packet
            // mode
            periphery::rf::SetTxType(periphery::rf::TxType::morse);
            periphery::rf::Morse(callSign);
        }
    }
} hafBeaconThread;
}
