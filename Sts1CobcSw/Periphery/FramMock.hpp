#pragma once


#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <strong_type/type.hpp>

#include <array>
#include <cstddef>
#include <cstdint>


namespace sts1cobcsw::fram
{
auto SetDoInitialize(void (*doInitializeFunction)()) -> void;
auto SetDoReadDeviceId(DeviceId (*doReadDeviceIdFunction)()) -> void;
auto SetDoActualBaudRate(std::int32_t (*doActualBaudRateFunction)()) -> void;
auto SetDoWriteTo(void (*doWriteToFunction)(
    Address address, void const * data, std::size_t nBytes, Duration timeout)) -> void;
auto SetDoReadFrom(void (*doReadFromFunction)(
    Address address, void * data, std::size_t nBytes, Duration timeout)) -> void;


// Empty do functions that do nothing; used to initialized function pointers
namespace empty
{
auto SetAllDoFunctions() -> void;

auto DoInitialize() -> void;
auto DoReadDeviceId() -> DeviceId;
auto DoActualBaudRate() -> std::int32_t;
auto DoWriteTo(Address address, void const * data, std::size_t nBytes, Duration timeout) -> void;
auto DoReadFrom(Address address, void * data, std::size_t nBytes, Duration timeout) -> void;
}


// Do functions that simulate the FRAM in RAM
namespace ram
{
extern std::array<Byte, value_of(memorySize)> memory;


auto SetAllDoFunctions() -> void;

auto DoInitialize() -> void;
auto DoReadDeviceId() -> DeviceId;
auto DoActualBaudRate() -> std::int32_t;
auto DoWriteTo(Address address, void const * data, std::size_t nBytes, Duration timeout) -> void;
auto DoReadFrom(Address address, void * data, std::size_t nBytes, Duration timeout) -> void;
}
}
