#pragma once

#include <littlefs/lfs.h>


namespace sts1cobcsw::fs
{
inline constexpr auto lfsCacheSize = 256;
// Our longest path should be strlen("/programs/65536.zip.lock") = 25 characters long
inline constexpr auto maxPathLength = 30;
extern lfs_config const lfsConfig;


auto Initialize() -> void;
}
