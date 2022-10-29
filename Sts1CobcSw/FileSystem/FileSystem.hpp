#pragma once


#include <littlefs/lfs.h>


namespace sts1cobcsw::fs
{
extern lfs_t lfs;
extern lfs_file_t lfsFile;
extern const lfs_config lfsConfig;

auto Initialize() -> void;
auto Mount() -> void;
auto Unmount() -> void;
}