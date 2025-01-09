#pragma once

#include <Sts1CobcSw/Serial/Byte.hpp>

#include <littlefs/lfs.h>

#include <cstdint>


namespace sts1cobcsw::fs
{

inline constexpr auto eraseValue = 0xFF_b;
inline constexpr auto initialCrcValue = 0;
inline constexpr auto crcSize = sizeof(std::uint32_t);
inline constexpr auto lfsCacheSize = 256 - crcSize;
// Our longest path should be strlen("/programs/65536.zip.lock") = 25 characters long
inline constexpr auto maxPathLength = 30;
extern lfs_config const lfsConfig;


auto Initialize() -> void;
}
