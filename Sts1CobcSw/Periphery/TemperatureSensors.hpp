#pragma once


#include <cstdint>


namespace sts1cobcsw::temperaturesensors
{
auto InitializeRf() -> void;
auto InitializeMcu() -> void;

auto ReadRf() -> std::uint16_t;
auto ReadMcu() -> std::uint16_t;

}
