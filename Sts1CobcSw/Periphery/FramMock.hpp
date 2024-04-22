#pragma once


#include <Sts1CobcSw/Periphery/Fram.hpp>


namespace sts1cobcsw::fram
{
auto SetDoInitialize(void (*doInitializeFunction)()) -> void;
auto SetDoReadDeviceId(DeviceId (*doReadDeviceIdFunction)()) -> void;
auto SetDoActualBaudRate(int32_t (*doActualBaudRateFunction)()) -> void;


// Default "do" functions that do nothing, used to initialized function pointers
auto DoInitializeDefault() -> void;
auto DoReadDeviceIdDefault() -> DeviceId;
auto DoActualBaudRateDefault() -> int32_t;


enum class MockMode
{
    ram,
    file
};

auto FramMockMode(MockMode mockMode) -> void;

namespace ram
{
auto DoInitialize() -> void;
auto DoReadDeviceId() -> DeviceId;
auto DoActualBaudRate() -> int32_t;
}

namespace file
{
auto DoInitialize() -> void;
auto DoReadDeviceId() -> DeviceId;
auto DoActualBaudRate() -> int32_t;
}
}
