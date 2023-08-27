#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/Periphery/RfNames.hpp>

#include <rodos_no_using_namespace.h>

#include <string_view>


namespace sts1cobcsw
{
constexpr auto callSign = periphery::rf::portableCallSign;
constexpr auto beaconStartTime = 5 * RODOS::SECONDS;
constexpr auto beaconPeriod = 60 * RODOS::SECONDS;


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
        RODOS::PRINTF("Initializing RF module\n");
        periphery::rf::Initialize(periphery::rf::TxType::morse);

        TIME_LOOP(beaconStartTime, beaconPeriod)
        {
            RODOS::PRINTF("\n");
            RODOS::PRINTF("Morsing call sign ...\n");
            periphery::rf::SetTxType(periphery::rf::TxType::morse);
            periphery::rf::Morse(callSign);

            RODOS::AT(RODOS::NOW() + 1 * RODOS::SECONDS);

            RODOS::PRINTF("\n");
            RODOS::PRINTF("Sending call sign ...\n");
            periphery::rf::SetTxType(periphery::rf::TxType::packet);
            periphery::rf::TransmitData(reinterpret_cast<std::uint8_t const *>(callSign.data()),
                                        callSign.size());
        }
    }
} hafBeaconThread;
}
