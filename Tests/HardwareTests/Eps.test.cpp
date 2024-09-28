#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>

#include <Sts1CobcSw/Periphery/Eps.hpp>

#include <rodos_no_using_namespace.h>

#include <cstddef>


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
        InitializeRfLatchupDisablePins();
    }


    void run() override
    {
        EnableRfLatchupProtection();

        PRINTF("\nEPS test\n\n");

        PRINTF("\n");
        eps::Initialize();
        PRINTF("EPS ADCs initialized\n");

        PRINTF("\n");
        RODOS::PRINTF("Reading sensor values ...\n");
        auto const referenceVoltage = 4.096;
        auto const resolution = 4096;
        auto const voltsPerBit = referenceVoltage / resolution;
        auto sensorValues = eps::Read();
        for(std::size_t i = 0; i < sensorValues.size(); ++i)
        {
            auto iAdc = static_cast<int>(i / eps::nChannels + 4);
            auto iChannel = static_cast<int>(i % eps::nChannels);
            RODOS::PRINTF("ADC %i channel %i:\n  Digital reading = %u\n  Measured voltage = %f V\n",
                          iAdc,
                          iChannel,
                          sensorValues[i],
                          sensorValues[i] * voltsPerBit);
        }
    }
} epsTest;
}
