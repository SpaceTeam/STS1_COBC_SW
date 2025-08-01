#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Sensors/Eps.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <rodos_no_using_namespace.h>

#include <array>


namespace sts1cobcsw
{
namespace
{
class EpsTest : public RODOS::StaticThread<>
{
public:
    EpsTest() : StaticThread("EpsTest")
    {}


private:
    void init() override
    {}


    void run() override
    {
        using RODOS::PRINTF;

        PRINTF("\nEPS test\n\n");

        PRINTF("\n");
        fram::Initialize();  // This is required for the persistent variables to work
        persistentVariables.Store<"epsIsWorking">(true);
        PRINTF("epsIsWorking = %s\n",
               persistentVariables.Load<"epsIsWorking">() ? "true" : "false");
        eps::InitializeAdcs();
        PRINTF("EPS ADCs initialized\n");

        PRINTF("\n");
        PRINTF("Reading ADCs ...\n");
        auto const referenceVoltage = 4.096;
        auto const resolution = 4096;
        auto const voltsPerBit = referenceVoltage / resolution;
        auto adcData = eps::ReadAdcs();
        PRINTF("ADC 4:\n");
        for(auto i = 0U; i < adcData.adc4.size(); ++i)
        {
            PRINTF("  channel = %2u,  raw value = %5u,  voltage = %1.3f V\n",
                   i,
                   adcData.adc4[i],
                   adcData.adc4[i] * voltsPerBit);
        }
        PRINTF("ADC 5:\n");
        for(auto i = 0U; i < adcData.adc5.size(); ++i)
        {
            PRINTF("  channel = %2u,  raw value = %5u,  voltage = %1.3f V\n",
                   i,
                   adcData.adc5[i],
                   adcData.adc5[i] * voltsPerBit);
        }
        PRINTF("ADC 6:\n");
        for(auto i = 0U; i < adcData.adc6.size(); ++i)
        {
            PRINTF("  channel = %2u,  raw value = %5u,  voltage = %1.3f V\n",
                   i,
                   adcData.adc6[i],
                   adcData.adc6[i] * voltsPerBit);
        }

        PRINTF("\n");
        PRINTF("Serializing and deserializing of ADC data ...");
        auto serializedData = Serialize(adcData);
        auto deserializedData = Deserialize<eps::AdcData>(serializedData);
        if(adcData == deserializedData)
        {
            PRINTF(" was successful\n");
        }
        else
        {
            PRINTF(" failed\n");
        }
    }
} epsTest;
}
}
