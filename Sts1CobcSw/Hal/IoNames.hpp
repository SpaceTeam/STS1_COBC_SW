#pragma once


#include <Sts1CobcSw/Hal/PinNames.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw::hal
{
inline constexpr auto led1Pin = pb12;
inline constexpr auto led2Pin = pb15;

inline constexpr auto watchdogClearPin = pa9;

inline constexpr auto epsBatteryGoodPin = pc15;
inline constexpr auto epsChargingPin = pc14;

inline constexpr auto eduEnablePin = pb0;
inline constexpr auto eduHeartbeatPin = pc5;
inline constexpr auto eduUpdatePin = pb1;

inline constexpr auto eduUartIndex = RODOS::UART_IDX1;
inline constexpr auto eduUartRxPin = pa10;
inline constexpr auto eduUartTxPin = pa15;

inline constexpr auto uciUartIndex = RODOS::UART_IDX2;
inline constexpr auto uciUartTxPin = pa2;
inline constexpr auto uciUartRxPin = pa3;

inline constexpr auto flashSpiIndex = RODOS::SPI_IDX1;
inline constexpr auto flashSpiSckPin = pa5;
inline constexpr auto flashSpiMisoPin = pa6;
inline constexpr auto flashSpiMosiPin = pa7;
inline constexpr auto flashCsPin = pb9;
inline constexpr auto flashWriteProtectionPin = pc4;

inline constexpr auto framEpsSpiIndex = RODOS::SPI_IDX3;
inline constexpr auto framEpsSpiSckPin = pc10;
inline constexpr auto framEpsSpiMisoPin = pc11;
inline constexpr auto framEpsSpiMosiPin = pc12;
inline constexpr auto framCsPin = pb13;

inline constexpr auto rfSpiIndex = RODOS::SPI_IDX2;
inline constexpr auto rfSpiSckPin = pc7;
inline constexpr auto rfSpiMisoPin = pc2;
inline constexpr auto rfSpiMosiPin = pc3;
inline constexpr auto rfCsPin = pa8;
inline constexpr auto rfNirqPin = pb6;
inline constexpr auto rfSdnPin = pb14;
inline constexpr auto rfGpio0Pin = pc6;
inline constexpr auto rfGpio1Pin = pc8;
inline constexpr auto rfPaEnablePin = pc9;

// We don't need/use NSS pins but due to a bug in Rodos we must specify one so we choose this
// unconnected pin.
inline constexpr auto spiNssDummyPin = pa0;
}
