#pragma once


#include <cstdint>


namespace sts1cobcsw::eps
{
using AdcValue = std::uint16_t;


struct SensorData
{
    AdcValue panelYMinusSolarCell1Voltage;
    AdcValue panelYMinusSolarCell2Voltage;
    AdcValue panelYPlusSolarCell1Voltage;
    AdcValue panelYPlusSolarCell2Voltage;
    AdcValue panelZMinusSolarCell1Voltage;
    AdcValue panelZPlusSolarCell1Voltage;
    AdcValue panelXPlusSolarCell1Voltage;
    AdcValue panelXPlusSolarCell2Voltage;
    AdcValue panelYMinusSolarCell1Current;
    AdcValue panelYMinusSolarCell2Current;
    AdcValue panelYPlusSolarCell1Current;
    AdcValue panelYPlusSolarCell2Current;
    AdcValue panelZMinusSolarCell1Current;
    AdcValue panelZPlusSolarCell1Current;
    AdcValue panelXPlusSolarCell1Current;
    AdcValue panelXPlusSolarCell2Current;

    AdcValue batteryPackVoltage;
    AdcValue batteryCenterTapVoltage;
    AdcValue batteryPackCurrent;
    AdcValue batteryTemperature;
    AdcValue panelYMinusTemperature;
    AdcValue panelYPlusTemperature;
    AdcValue panelZMinusTemperature;
    AdcValue panelZPlusTemperature;
    AdcValue panelXPlusTemperature;
    AdcValue epsOutputCurrentToVbus;

    AdcValue mpptBusCurrent;
    AdcValue panelZMinusMppt1Current;
    AdcValue panelYMinusMppt2Current;
    AdcValue panelYPlusMppt1Current;
    AdcValue panelYPlusMppt2Current;
    AdcValue panelYMinusMppt1Current;
    AdcValue panelZPlusMppt1Current;
    AdcValue panelXPlusMppt1Current;
    AdcValue panelXPlusMppt2Current;
    AdcValue mpptBusVoltage;
};


auto Initialize() -> void;
[[nodiscard]] auto Read() -> SensorData;
auto ResetAdcRegisters() -> void;
auto ClearFifos() -> void;
}
