#pragma once

#include <littlefs/lfs.h>


namespace sts1cobcsw::fs
{
extern lfs_config const lfsConfig;


auto Initialize() -> void;
}
