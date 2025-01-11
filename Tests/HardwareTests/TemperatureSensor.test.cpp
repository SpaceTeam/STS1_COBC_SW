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
    }


    void run() override
    {
        using RODOS::PRINTF;

        EnableRfLatchupProtection();

        PRINTF("\nMCU temperature sensor test\n\n");

        auto const conversionFactor = 0.0806;  // °C/bit
        auto const offset = -50;               // °C at 0 V
        TIME_LOOP(0, 1000 * RODOS::MILLISECONDS)
        {
            auto temperature = rftemperaturesensor::Read();
            PRINTF("MCU raw value   = %5d\n", temperature);
            PRINTF("MCU temperature = %5.1f deg C\n", temperature * conversionFactor + offset);
        }
    }
} termperatureSensorTest;
}
