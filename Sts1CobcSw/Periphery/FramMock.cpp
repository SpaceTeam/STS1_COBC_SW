#include <Sts1CobcSw/Periphery/FramMock.hpp>

#include <array>
#include <cstring>
#include <fstream>  // IWYU pragma: keep
#include <iostream>


namespace sts1cobcsw::fram
{
constexpr auto framSize = (1U << 20U);
auto ramSimulation = std::array<uint8_t, framSize>{};
MockMode mockDevice = MockMode::ram;
constexpr auto mockFilename = "FramMock.bin";

auto doInitialize = empty::DoInitialize;
auto doReadDeviceId = empty::DoReadDeviceId;
auto doActualBaudRate = empty::DoActualBaudRate;


// --- Mocked functions ---

auto Initialize() -> void
{
    return doInitialize();
}


auto ReadDeviceId() -> DeviceId
{
    return doReadDeviceId();
}


auto ActualBaudRate() -> int32_t
{
    return doActualBaudRate();
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


void SetDoActualBaudRate(int32_t (*doActualBaudRateFunction)())
{
    doActualBaudRate = doActualBaudRateFunction;
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


auto DoActualBaudRate() -> int32_t
{
    return 0;
}
}


auto FramMockMode(MockMode mockMode) -> void
{
    mockDevice = mockMode;
}



namespace internal
{
// TODO: This must also forward to a do function which can be set with a SetDoWriteTo function
auto WriteTo(Address address, void const * data, std::size_t nBytes) -> void
{
    if(mockDevice == MockMode::file)
    {
        std::ofstream file(mockFilename, std::ios::binary | std::ios_base::ate);
        if(not file)
        {
            std::cerr << "Failed to open file " << mockFilename << " for writing." << '\n';
            return;
        }

        file.seekp(address);
        file.write(static_cast<char const *>(data), static_cast<std::streamsize>(nBytes));
        file.close();
    }
    else
    {
        // FIXME: Fix pointer arithmetic and out of bounds access
        std::memcpy(ramSimulation.data() + address, data, nBytes);
    }
}


// TODO: This must also forward to a do function which can be set with a SetDoReadFrom function
auto ReadFrom(Address address, void * data, std::size_t nBytes) -> void
{
    if(mockDevice == MockMode::file)
    {
        std::ifstream file(mockFilename, std::ios::binary);
        if(not file)
        {
            std::cerr << "Failed to open file " << mockFilename << " for reading." << '\n';
            return;
        }

        file.clear();
        file.seekg(address);
        file.read(static_cast<char *>(data), static_cast<std::streamsize>(nBytes));
        file.close();
    }
    else
    {
        std::memcpy(data, ramSimulation.data() + address, nBytes);
    }
}
}
}
