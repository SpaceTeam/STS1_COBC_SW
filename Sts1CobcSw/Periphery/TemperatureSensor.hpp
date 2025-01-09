#pragma once


#include <cstdint>


namespace sts1cobcsw::rftemperaturesensor
{
auto Initialize() -> void;
auto Read() -> std::uint16_t;

auto InitializeStm32Temperature() -> void;
auto ReadStm32Temperature() -> std::uint16_t;
}
