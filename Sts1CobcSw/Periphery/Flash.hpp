#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <cstdint>


namespace sts1cobcsw::periphery::flash
{
using serial::Byte;


// TODO: Make it deserializable
struct JedecId
{
    std::uint8_t manufacturerId = 0U;
    std::uint16_t deviceId = 0U;
};


// TODO: Proper error handling/return type
[[nodiscard]] auto Initialize() -> std::int32_t;
[[nodiscard]] auto ReadJedecId() -> JedecId;
[[nodiscard]] auto ReadStatusRegister(int8_t registerNo) -> Byte;

// TODO: Block read, write, program, and sync functions
}