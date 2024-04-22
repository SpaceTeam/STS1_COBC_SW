#include <Sts1CobcSw/Periphery/FramMock.hpp>

#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <vector>


namespace sts1cobcsw::fram
{

auto ramSimulation = std::array<uint8_t, 1048576>{};
MockMode mockDevice = MockMode::ram;
constexpr auto mockFilename = "FramMock.bin";

auto FramMockMode(MockMode mockMode) -> void
{
    mockDevice = mockMode;
}

// Default do functions, doing nothing
auto DoInitializeDefault() -> void
{
}

auto DoReadDeviceIdDefault() -> DeviceId
{
    return DeviceId{};
}

auto DoActualBaudRateDefault() -> int32_t
{
    return 0;
}


auto doInitialize = DoInitializeDefault;
auto doReadDeviceId = DoReadDeviceIdDefault;
auto doActualBaudRate = DoActualBaudRateDefault;


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


namespace internal
{
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
        // FIXME: Fix pointer arithmetic
        std::memcpy(ramSimulation.data() + address, data, nBytes);
    }
}

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
