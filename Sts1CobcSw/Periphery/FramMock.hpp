#pragma once


#include <Sts1CobcSw/Periphery/Fram.hpp>

#include <cstdint>


namespace sts1cobcsw::fram
{
auto SetDoInitialize(void (*doInitializeFunction)()) -> void;
auto SetDoReadDeviceId(DeviceId (*doReadDeviceIdFunction)()) -> void;
auto SetDoActualBaudRate(std::int32_t (*doActualBaudRateFunction)()) -> void;
auto SetDoWriteTo(void (*doWriteToFunction)(
    Address address, void const * data, std::size_t nBytes, std::int64_t timeout)) -> void;
auto SetDoReadFrom(void (*doReadFromFunction)(
    Address address, void * data, std::size_t nBytes, std::int64_t timeout)) -> void;


// Empty do functions that do nothing; used to initialized function pointers
namespace empty
{
auto DoInitialize() -> void;
auto DoReadDeviceId() -> DeviceId;
auto DoActualBaudRate() -> std::int32_t;
auto DoWriteTo(Address address, void const * data, std::size_t nBytes, std::int64_t timeout)
    -> void;
auto DoReadFrom(Address address, void * data, std::size_t nBytes, std::int64_t timeout) -> void;
}


// TODO: Implement ram::do functions
// Do functions
namespace ram
{
auto DoInitialize() -> void;
auto DoReadDeviceId() -> DeviceId;
auto DoActualBaudRate() -> std::int32_t;
}
}
