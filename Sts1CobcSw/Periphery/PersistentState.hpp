#pragma once


#include <cstdint>


// This is just a dummy implementation that does not store anything in a persistent memory.
namespace sts1cobcsw::persistentstate
{
// Must be called once in a thread's init() function
auto Initialize() -> void;

// Getters
[[nodiscard]] auto NotOkCounter() -> std::int8_t;
[[nodiscard]] auto ActiveFirmwareImage() -> std::int8_t;
[[nodiscard]] auto BackupFirmwareImage() -> std::int8_t;

[[nodiscard]] auto AntennasShouldBeDeployed() -> bool;
[[nodiscard]] auto TxIsOn() -> bool;
[[nodiscard]] auto EduShouldBePowered() -> bool;
[[nodiscard]] auto UtcOffset() -> std::int32_t;

// Setters
auto NotOkCounter(std::int8_t value) -> void;
auto ActiveFirmwareImage(std::int8_t value) -> void;
auto BackupFirmwareImage(std::int8_t value) -> void;

auto AntennasShouldBeDeployed(bool value) -> void;
auto TxIsOn(bool value) -> void;
auto EduShouldBePowered(bool value) -> void;
auto UtcOffset(std::int32_t value) -> void;
}
