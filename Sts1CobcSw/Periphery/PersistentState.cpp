#include <Sts1CobcSw/Periphery/PersistentState.hpp>


// This is just a dummy implementation that does not store anything in a persistent memory.
namespace sts1cobcsw::persistentstate
{
// TODO: Think about how large the types really need to be
// TODO: Maybe use std::optionl<T> or just have an isInitialized flag or something
// Bootloader stuff
std::int8_t notOkCounter = -1;         // NOLINT(cert-err58-cpp)
std::int8_t activeFirmwareImage = -1;  // NOLINT(cert-err58-cpp)
std::int8_t backupFirmwareImage = -1;  // NOLINT(cert-err58-cpp)

// COBC state
auto antennasShouldBeDeployed = true;
auto txIsOn = true;
auto eduShouldBePowered = false;
std::int32_t utcOffset = 0;  // NOLINT(cert-err58-cpp)

// Error counters
std::uint16_t flashErrorCounter = 0;
std::uint16_t rfErrorCounter = 0;

// Periphery state
auto framIsWorking = true;
auto epsIsWorking = true;
auto flashIsWorking = true;
auto rfIsWorking = true;

// TODO: Add thresholds


auto Initialize() -> void
{
    // Load all variables from FRAM
}


// Getters
auto NotOkCounter() -> std::int8_t
{
    return notOkCounter;
}


auto ActiveFirmwareImage() -> std::int8_t
{
    return activeFirmwareImage;
}


auto BackupFirmwareImage() -> std::int8_t
{
    return backupFirmwareImage;
}


auto AntennasShouldBeDeployed() -> bool
{
    return antennasShouldBeDeployed;
}


auto TxIsOn() -> bool
{
    return txIsOn;
}


auto EduShouldBePowered() -> bool
{
    return eduShouldBePowered;
}


auto UtcOffset() -> std::int32_t
{
    return utcOffset;
}


auto FlashErrorCounter() -> std::uint16_t
{
    return flashErrorCounter;
}


auto RfErrorCounter() -> std::uint16_t
{
    return rfErrorCounter;
}


auto EpsIsWorking() -> bool
{
    return epsIsWorking;
}


auto FlashIsWorking() -> bool
{
    return flashIsWorking;
}


auto FramIsWorking() -> bool
{
    return framIsWorking;
}


auto RfIsWorking() -> bool
{
    return rfIsWorking;
}


// Setters
auto NotOkCounter(std::int8_t value) -> void
{
    notOkCounter = value;
}


auto ActiveFirmwareImage(std::int8_t value) -> void
{
    activeFirmwareImage = value;
}


auto BackupFirmwareImage(std::int8_t value) -> void
{
    backupFirmwareImage = value;
}


auto AntennasShouldBeDeployed(bool value) -> void
{
    antennasShouldBeDeployed = value;
}


auto TxIsOn(bool value) -> void
{
    txIsOn = value;
}


auto EduShouldBePowered(bool value) -> void
{
    eduShouldBePowered = value;
}


auto UtcOffset(std::int32_t value) -> void
{
    utcOffset = value;
}


auto FlashErrorCounter(std::uint16_t value) -> void
{
    flashErrorCounter = value;
}


auto RfErrorCounter(std::uint16_t value) -> void
{
    rfErrorCounter = value;
}


auto EpsIsWorking(bool value) -> void
{
    epsIsWorking = value;
}


auto FlashIsWorking(bool value) -> void
{
    flashIsWorking = value;
}


auto FramIsWorking(bool value) -> void
{
    framIsWorking = value;
}


auto RfIsWorking(bool value) -> void
{
    rfIsWorking = value;
}
}
