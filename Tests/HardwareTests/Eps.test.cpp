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
        static constexpr auto nSensorValues = eps::nChannels * eps::nAdcs;
        static constexpr auto referenceVoltage = 4.096;
        static constexpr auto resolution = 4096;

        // ADCs 1-3 belong to EDU, COBC has 4-6
        static constexpr auto adcIdOffset = 4;

        RODOS::PRINTF("Reading...\n");
        auto sensorData = eps::Read();
        void const * dataPointer = sensorData.data();
        RODOS::PRINTF("Result:\n");
        for(size_t i = 0; i < nSensorValues; i++)
        {
            std::uint16_t value = 0U;
            dataPointer = DeserializeFrom<std::endian::big>(dataPointer, &value);
            auto measuredVoltage = value * (referenceVoltage / resolution);
            auto adc = static_cast<int>(i / eps::nChannels) + adcIdOffset;
            auto channel = static_cast<int>(i % eps::nChannels);
            RODOS::PRINTF("ADC %i Channel %i:\n  Digital reading = %u\n  Measured voltage = %f\n",
                          adc,
                          channel,
                          value,
                          measuredVoltage);
        }
        RODOS::PRINTF("\n");
    }
} epsTest;
}
