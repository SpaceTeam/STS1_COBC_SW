#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>

#include <Sts1CobcSw/Periphery/TemperatureSensor.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
class TermperatureSensorTest : public RODOS::StaticThread<>
{
public:
    TermperatureSensorTest() : StaticThread("TermperatureSensorTest")
    {
    }


private:
    void init() override
    {
        InitializeRfLatchupDisablePins();
        rftemperaturesensor::Initialize();

        rftemperaturesensor::InitializeStm32Temperature();
    }


    void run() override
    {
        using RODOS::PRINTF;

        EnableRfLatchupProtection();

        PRINTF("\nRF temperature sensor test\n\n");

        PRINTF("\nMCU temperature sensor test\n\n");

        auto const conversionFactor = 0.0806;  // °C/bit
        auto const offset = -50;               // °C at 0 V
        TIME_LOOP(0, 1000 * RODOS::MILLISECONDS)
        {
            auto temperature = rftemperaturesensor::Read();
            PRINTF("raw value   = %5d\n", temperature);
            PRINTF("temperature = %5.1f deg C\n", temperature * conversionFactor + offset);

            auto temperatureStm32 = rftemperaturesensor::ReadStm32Temperature();
            PRINTF("MCU raw value   = %5d\n", temperatureStm32);
            PRINTF("MCU temperature? = %5.1f deg C\n", temperature * conversionFactor + offset);
        }
    }
} termperatureSensorTest;
}
