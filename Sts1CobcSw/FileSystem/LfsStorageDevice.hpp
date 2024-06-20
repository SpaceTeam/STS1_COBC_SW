#pragma once

#include <littlefs/lfs.h>


namespace sts1cobcsw::fs
{
inline constexpr auto lfsCacheSize = 256;
extern lfs_config const lfsConfig;


auto Initialize() -> void;
}
