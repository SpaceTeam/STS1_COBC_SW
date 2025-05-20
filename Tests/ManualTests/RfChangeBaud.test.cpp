#include <Tests/HardwareSetup/RfLatchupProtection.hpp>

#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Rf/Rf.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <strong_type/difference.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <cstdint>
#include <type_traits>


namespace sts1cobcsw
{
using RODOS::PRINTF;

auto led1GpioPin = hal::GpioPin(hal::led1Pin);


class RfRxTxBaudTest : public RODOS::StaticThread<>
{
public:
    RfRxTxBaudTest() : StaticThread("RfRxTxBaudTest")
    {
    }


private:
    void init() override
    {
        led1GpioPin.SetDirection(hal::PinDirection::out);
        led1GpioPin.Reset();
    }


    void run() override
    {
        PRINTF("\nRF change RX TX Baud test\n\n");

        rf::Initialize(rf::TxType::packet);
        rf::EnableTx();
        PRINTF("RF module initialized, TX enabled\n");


        TIME_LOOP(0, 1000 * RODOS::MILLISECONDS)
        {
            auto temperature = rftemperaturesensor::Read();
            PRINTF("raw value   = %5d\n", temperature);
            PRINTF("temperature = %5.1f deg C\n", temperature * conversionFactor + offset);
        }
    }
} termperatureSensorTest;
}
