#pragma once

#include <Sts1CobcSw/Hal/PinNames.hpp>

namespace sts1cobcsw::hal
{
inline constexpr auto ledPin = pa13;

inline constexpr auto eduUartIndex = RODOS::UART_IDX1;
inline constexpr auto eduUartRxPin = pa10;
inline constexpr auto eduUartTxPin = pa15;

inline constexpr auto uciUartIndex = RODOS::UART_IDX2;
inline constexpr auto uciUartTxPin = pa2;
inline constexpr auto uciUartRxPin = pa3;

inline constexpr auto eduUpdatePin = pb1;
inline constexpr auto epsChargingPin = pc14;
inline constexpr auto epsBatteryGoodPin = pc15;
inline constexpr auto eduHeartbeatPin = pc5;
inline constexpr auto eduEnabledPin = pb0;

inline constexpr auto flashSpiIndex = RODOS::SPI_IDX1;
inline constexpr auto flashSpiSckPin = pa5;
inline constexpr auto flashSpiMisoPin = pa6;
inline constexpr auto flashSpiMosiPin = pa7;

inline constexpr auto framSpiIndex = RODOS::SPI_IDX2;
inline constexpr auto framSpiSckPin = pc7;
inline constexpr auto framSpiMisoPin = pc2;
inline constexpr auto framSpiMosiPin = pc3;

inline constexpr auto cobcSpiIndex = RODOS::SPI_IDX3;
inline constexpr auto cobcSpiSckPin = pc10;
inline constexpr auto cobcSpiMisoPin = pc11;
inline constexpr auto cobcSpiMosiPin = pc12;

inline constexpr auto rfSpiIndex = RODOS::SPI_IDX4;
inline constexpr auto rfSpiSckPin = pb13;
inline constexpr auto rfSpiMisoPin = pa11;
inline constexpr auto rfSpiMosiPin = pa1;
}
