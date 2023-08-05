#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/Periphery/RfNames.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <rodos/support/support-libs/random.h>
#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
using periphery::rf::portableCallSign;
using serial::Byte;


struct Beacon
{
    std::int64_t timestamp = 0U;
    std::uint64_t randomData = 0U;
};


namespace serial
{
template<>
inline constexpr std::size_t serialSize<Beacon> =
    totalSerialSize<decltype(Beacon::timestamp), decltype(Beacon::randomData)>;
}


auto SerializeTo(void * destination, Beacon const & data) -> void *;


class MftBeaconThread : public RODOS::StaticThread<>
{
public:
    MftBeaconThread() : StaticThread("MftBeaconThread")
    {
    }

private:
    void init() override
    {
    }


    void run() override
    {
        RODOS::PRINTF("Initializing RF module in packet mode\n");
        periphery::rf::Initialize(periphery::rf::TxType::packet);

        RODOS::setRandSeed(static_cast<std::uint64_t>(RODOS::NOW()));

        TIME_LOOP(5 * RODOS::SECONDS, 10 * RODOS::SECONDS)
        {
            RODOS::PRINTF("\n");
            RODOS::PRINTF("Sending call sign ...\n");
            periphery::rf::TransmitData(
                reinterpret_cast<std::uint8_t const *>(portableCallSign.data()),
                portableCallSign.size());

            RODOS::PRINTF("Sending 'test' ...\n");
            std::uint8_t testMessage[] = "test";
            periphery::rf::TransmitData(&testMessage[0], std::size(testMessage));

            RODOS::PRINTF("Sending beacon ...\n");
            auto serialBeacon = serial::Serialize(
                Beacon{.timestamp = RODOS::NOW(), .randomData = RODOS::uint64Rand()});
            periphery::rf::TransmitData(reinterpret_cast<std::uint8_t const *>(serialBeacon.data()),
                                        serialBeacon.size());
        }
    }
} mftBeaconThread;


auto SerializeTo(void * destination, Beacon const & data) -> void *
{
    destination = serial::SerializeTo(destination, data.timestamp);
    destination = serial::SerializeTo(destination, data.randomData);
    return destination;
}
}
