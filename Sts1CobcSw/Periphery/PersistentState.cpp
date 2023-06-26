#include <Sts1CobcSw/Periphery/PersistentState.hpp>


// This is just a dummy implementation that does not store anything in a persistent memory.
namespace sts1cobcsw::periphery::persistentstate
{
namespace ts = type_safe;

using ts::operator""_i8;
using ts::operator""_i32;

// TODO: Think about how large the types really need to be
// TODO: Maybe use std::optionl<T> or just have an isInitialized flag or something
// Bootloader stuff
auto notOkCounter = -1_i8;         // NOLINT(cert-err58-cpp)
auto activeFirmwareImage = -1_i8;  // NOLINT(cert-err58-cpp)
auto backupFirmwareImage = -1_i8;  // NOLINT(cert-err58-cpp)

// COBC state
ts::bool_t antennasShouldBeDeployed = true;
ts::bool_t txIsOn = true;
ts::bool_t eduShouldBePowered = false;
auto utcOffset = 0_i32;  // NOLINT(cert-err58-cpp)

// TODO: Add thresholds


auto Initialize() -> void
{
    // Load all variables from FRAM
}


// Getters
auto NotOkCounter() -> type_safe::int8_t
{
    return notOkCounter;
}


auto ActiveFirmwareImage() -> type_safe::int8_t
{
    return activeFirmwareImage;
}


auto BackupFirmwareImage() -> type_safe::int8_t
{
    return backupFirmwareImage;
}


auto AntennasShouldBeDeployed() -> type_safe::bool_t
{
    return antennasShouldBeDeployed;
}


auto TxIsOn() -> type_safe::bool_t
{
    return txIsOn;
}


auto EduShouldBePowered() -> type_safe::bool_t
{
    return eduShouldBePowered;
}


auto UtcOffset() -> type_safe::int32_t
{
    return utcOffset;
}


// Setters
auto NotOkCounter(type_safe::int8_t value) -> void
{
    notOkCounter = value;
}


auto ActiveFirmwareImage(type_safe::int8_t value) -> void
{
    activeFirmwareImage = value;
}


auto BackupFirmwareImage(type_safe::int8_t value) -> void
{
    backupFirmwareImage = value;
}


auto AntennasShouldBeDeployed(type_safe::bool_t value) -> void
{
    antennasShouldBeDeployed = value;
}


auto TxIsOn(type_safe::bool_t value) -> void
{
    txIsOn = value;
}


auto EduShouldBePowered(type_safe::bool_t value) -> void
{
    eduShouldBePowered = value;
}


auto UtcOffset(type_safe::int32_t value) -> void
{
    utcOffset = value;
}
}