#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/Fram/FramMock.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>

#include <strong_type/type.hpp>

#include <cstring>


namespace sts1cobcsw::fram
{
EdacVariable<bool> framIsWorking(true);


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
auto WriteTo(Address address, void const * data, std::size_t nBytes, Duration timeout) -> void
{
    return doWriteTo(address, data, nBytes, timeout);
}


auto ReadFrom(Address address, void * data, std::size_t nBytes, Duration timeout) -> void
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
    Address address, void const * data, std::size_t nBytes, Duration timeout)) -> void
{
    doWriteTo = doWriteToFunction;
}


auto SetDoReadFrom(void (*doReadFromFunction)(
    Address address, void * data, std::size_t nBytes, Duration timeout)) -> void
{
    doReadFrom = doReadFromFunction;
}


// --- Predefined do functions ---

namespace empty
{
auto SetAllDoFunctions() -> void
{
    SetDoInitialize(DoInitialize);
    SetDoReadDeviceId(DoReadDeviceId);
    SetDoActualBaudRate(DoActualBaudRate);
    SetDoWriteTo(DoWriteTo);
    SetDoReadFrom(DoReadFrom);
}


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
               [[maybe_unused]] Duration timeout) -> void
{
}


auto DoReadFrom([[maybe_unused]] Address address,
                [[maybe_unused]] void * data,
                [[maybe_unused]] std::size_t nBytes,
                [[maybe_unused]] Duration timeout) -> void
{
}
}


namespace ram
{
std::array<Byte, value_of(memorySize)> memory{};


auto SetAllDoFunctions() -> void
{
    SetDoInitialize(DoInitialize);
    SetDoReadDeviceId(DoReadDeviceId);
    SetDoActualBaudRate(DoActualBaudRate);
    SetDoWriteTo(DoWriteTo);
    SetDoReadFrom(DoReadFrom);
}


auto DoInitialize() -> void
{
}


auto DoReadDeviceId() -> DeviceId
{
    return fram::correctDeviceId;
}


auto DoActualBaudRate() -> std::int32_t
{
    return 6'000'000;  // NOLINT(*magic-numbers*)
}


auto DoWriteTo(Address address,
               void const * data,
               std::size_t nBytes,
               [[maybe_unused]] Duration timeout) -> void
{
    std::memcpy(memory.data() + value_of(address), data, nBytes);
}


auto DoReadFrom(Address address, void * data, std::size_t nBytes, [[maybe_unused]] Duration timeout)
    -> void
{
    std::memcpy(data, memory.data() + value_of(address), nBytes);
}
}
}
