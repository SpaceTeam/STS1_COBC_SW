#pragma once


#include <type_safe/types.hpp>

#include <cstdint>


// This is just a dummy implementation that does not store anything in a persistent memory.
namespace sts1cobcsw::periphery::persistentstate
{
// Must be called once in a thread's init() function
auto Initialize() -> void;

// Getters
[[nodiscard]] auto NotOkCounter() -> type_safe::int8_t;
[[nodiscard]] auto ActiveFirmwareImage() -> type_safe::int8_t;
[[nodiscard]] auto BackupFirmwareImage() -> type_safe::int8_t;

[[nodiscard]] auto AntennasShouldBeDeployed() -> type_safe::bool_t;
[[nodiscard]] auto TxIsOn() -> type_safe::bool_t;
[[nodiscard]] auto EduShouldBePowered() -> type_safe::bool_t;
[[nodiscard]] auto UtcOffset() -> type_safe::int32_t;

// Setters
auto NotOkCounter(type_safe::int8_t) -> void;
auto ActiveFirmwareImage(type_safe::int8_t) -> void;
auto BackupFirmwareImage(type_safe::int8_t) -> void;

auto AntennasShouldBeDeployed(type_safe::bool_t) -> void;
auto TxIsOn(type_safe::bool_t) -> void;
auto EduShouldBePowered(type_safe::bool_t) -> void;
auto UtcOffset(type_safe::int32_t) -> void;
}