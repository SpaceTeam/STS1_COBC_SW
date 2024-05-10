#include <Sts1CobcSw/Periphery/FramMock.hpp>

#include <array>
#include <cstring>
#include <fstream>  // IWYU pragma: keep
#include <iostream>


namespace sts1cobcsw::fram
{
constexpr auto framSize = (1U << 20U);
auto ramSimulation = std::array<uint8_t, framSize>{};

auto doInitialize = empty::DoInitialize;
auto doReadDeviceId = empty::DoReadDeviceId;
auto doActualBaudRate = empty::DoActualBaudRate;
auto doWriteTo = empty::DoWriteTo;
auto doReadFrom = empty::DoReadFrom;


// --- Mocked functions ---

auto Initialize() -> void
{
    return doInitialize();
}


auto ReadDeviceId() -> DeviceId
{
    return doReadDeviceId();
}


auto ActualBaudRate() -> std::int32_t
{
    return doActualBaudRate();
}


namespace internal
{
auto WriteTo(Address address, void const * data, std::size_t nBytes, std::int64_t timeout) -> void
{
    return doWriteTo(address, data, nBytes, timeout);
}


auto ReadFrom(Address address, void * data, std::size_t nBytes, std::int64_t timeout) -> void
{
    return doReadFrom(address, data, nBytes, timeout);
}
}


// --- Set functions ---

auto SetDoInitialize(void (*doInitializeFunction)()) -> void
{
    doInitialize = doInitializeFunction;
}


auto SetDoReadDeviceId(DeviceId (*doReadDeviceIdFunction)()) -> void
{
    doReadDeviceId = doReadDeviceIdFunction;
}


void SetDoActualBaudRate(std::int32_t (*doActualBaudRateFunction)())
{
    doActualBaudRate = doActualBaudRateFunction;
}


auto SetDoWriteTo(void (*doWriteToFunction)(
    Address address, void const * data, std::size_t nBytes, std::int64_t timeout)) -> void
{
    doWriteTo = doWriteToFunction;
}


auto SetDoReadFrom(void (*doReadFromFunction)(
    Address address, void * data, std::size_t nBytes, std::int64_t timeout)) -> void
{
    doReadFrom = doReadFromFunction;
}


// --- Predefined do functions ---

namespace empty
{
auto DoInitialize() -> void
{
}


auto DoReadDeviceId() -> DeviceId
{
    return DeviceId{};
}


auto DoActualBaudRate() -> std::int32_t
{
    return 0;
}


auto DoWriteTo([[maybe_unused]] Address address,
               [[maybe_unused]] void const * data,
               [[maybe_unused]] std::size_t nBytes,
               [[maybe_unused]] std::int64_t timeout) -> void
{
}


auto DoReadFrom([[maybe_unused]] Address address,
                [[maybe_unused]] void * data,
                [[maybe_unused]] std::size_t nBytes,
                [[maybe_unused]] std::int64_t timeout) -> void
{
}
}
}
