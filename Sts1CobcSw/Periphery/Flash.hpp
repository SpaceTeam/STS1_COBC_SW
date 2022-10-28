#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <cstdint>


namespace sts1cobcsw::periphery::flash
{
using serial::Byte;
using serial::operator""_b;


// TODO: Make it deserializable
struct JedecId
{
    std::uint8_t manufacturerId = 0U;
    std::uint16_t deviceId = 0U;
};


// TODO: Turn this into bit field/set or something
struct StatusRegisters
{
    Byte one = 0_b;
    Byte two = 0_b;
    Byte three = 0_b;
};


// TODO: Proper error handling/return type
[[nodiscard]] auto Initialize() -> std::int32_t;
[[nodiscard]] auto ReadJedecId() -> JedecId;
[[nodiscard]] auto ReadStatusRegisters() -> StatusRegisters;
// TODO: Block read, write, program, and sync functions
}