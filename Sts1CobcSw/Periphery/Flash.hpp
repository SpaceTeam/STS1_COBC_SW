#pragma once


#include <cstdint>


namespace sts1cobcsw::periphery::flash
{
// TODO: Make it deserializable
struct JedecId
{
    std::uint8_t manufacturerId = 0U;
    std::uint16_t deviceId = 0U;
};


// TODO: Proper error handling/return type
[[nodiscard]] auto Initialize() -> std::int32_t;
[[nodiscard]] auto ReadJedecId() -> JedecId;
}