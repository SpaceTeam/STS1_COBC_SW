#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>

#include <Sts1CobcSw/Periphery/TemperatureSensors.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
auto ConvertToMcuTemperature(std::uint16_t adcValue) -> double
{
    static constexpr auto conversionFactor = 3.3 / (1U << 12U);  // V/bit
    static constexpr auto T0 = 25.0;              // °C        // NOLINT(*identifier-naming)
    static constexpr auto V25 = 0.76;             // V @ 25°C  // NOLINT(*identifier-naming)
    static constexpr auto averageSlope = 0.0025;  // V/°C
    return (static_cast<double>(adcValue) * conversionFactor - V25) / averageSlope + T0;
}


auto ConvertToRfTemperature(std::uint16_t adcValue) -> double
{
    static constexpr auto conversionFactor = 3.3 / (1U << 12U) / 0.01;  // °C/bit
    static constexpr auto offset = -50.0;                               // °C at 0 V
    return static_cast<double>(adcValue) * conversionFactor + offset;
}


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
    }


    void run() override
    {
        using RODOS::PRINTF;

        EnableRfLatchupProtection();

        PRINTF("\nTemperature sensor test\n\n");

        temperaturesensors::Initialize();

        TIME_LOOP(0, 1000 * RODOS::MILLISECONDS)
        {
            auto adcValue = temperaturesensors::ReadRfTemperature();
            PRINTF("RF raw value    = %5d\n", adcValue);
            PRINTF("RF temperature  = %5.1f deg C\n", ConvertToRfTemperature(adcValue));
            adcValue = temperaturesensors::ReadMcuTemperature();
            PRINTF("MCU raw value   = %5d\n", adcValue);
            PRINTF("MCU temperature = %5.1f deg C\n", ConvertToMcuTemperature(adcValue));
            PRINTF("\n");
        }
    }
} termperatureSensorTest;
}
