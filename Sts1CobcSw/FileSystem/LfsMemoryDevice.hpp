#pragma once


#include <littlefs/lfs.h>

#include <cstdint>


namespace sts1cobcsw::fs
{
inline constexpr auto crcSize = sizeof(std::uint32_t);
inline constexpr auto lfsCacheSize = 256 - crcSize;
// Our longest path should be strlen("/results/65536_4294967295.cpio.lock") = 35 characters long
inline constexpr auto maxPathLength = 35;
extern lfs_config const lfsConfig;


auto Initialize() -> void;
}
