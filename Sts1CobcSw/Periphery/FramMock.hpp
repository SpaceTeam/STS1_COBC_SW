#pragma once


#include <Sts1CobcSw/Periphery/Fram.hpp>

#include <cstdint>


namespace sts1cobcsw::fram
{
enum class MockMode
{
    ram,
    file
};


// TODO: Add SetDoWriteTo and SetDoReadFrom functions
auto SetDoInitialize(void (*doInitializeFunction)()) -> void;
auto SetDoReadDeviceId(DeviceId (*doReadDeviceIdFunction)()) -> void;
auto SetDoActualBaudRate(std::int32_t (*doActualBaudRateFunction)()) -> void;

namespace empty
{
    // Empty do functions that do nothing, used to initialized function pointers
    auto DoInitialize()->void;
    auto DoReadDeviceId()->DeviceId;
    auto DoActualBaudRate()->std::int32_t;
}

// TODO: Remove this
auto FramMockMode(MockMode mockMode) -> void;

// TODO: Implement ram::do functions
namespace ram
{
auto DoInitialize() -> void;
auto DoReadDeviceId() -> DeviceId;
auto DoActualBaudRate() -> std::int32_t;
}


// TODO: Since this isn't fully implemented yet, remove it. The RAM mock should be enough.
namespace file
{
auto DoInitialize() -> void;
auto DoReadDeviceId() -> DeviceId;
auto DoActualBaudRate() -> std::int32_t;
}
}
