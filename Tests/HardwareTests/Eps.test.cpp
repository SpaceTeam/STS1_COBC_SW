#include <Sts1CobcSw/Periphery/Eps.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
using RODOS::PRINTF;


class EpsTest : public RODOS::StaticThread<>
{
public:
    EpsTest() : StaticThread("EpsTest")
    {
    }


private:
    void init() override
    {
    }


    void run() override
    {
        PRINTF("\nEPS test\n\n");
        eps::Initialize();
        PRINTF("EPS ADCs initialized\n");
        PRINTF("\n");
        constexpr auto nSensorValues = eps::nChannels * eps::nAdcs;
        
        TIME_LOOP(0, 2 * RODOS::SECONDS)
        {
            RODOS::PRINTF("Reading...\n");
            auto sensorData = eps::Read();
            void const * dataPointer = sensorData.data();
            RODOS::PRINTF("Result:\n");
            for(size_t i = 0; i < nSensorValues; i++)
            {
                std::uint16_t value = 0U;
                dataPointer = DeserializeFrom<std::endian::big>(dataPointer, &value);
                RODOS::PRINTF("%u", value);
            }
            RODOS::PRINTF("\n");
        }
    }
} epsTest;
}
