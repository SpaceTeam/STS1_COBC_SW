#pragma once

#include <littlefs/lfs.h>


namespace sts1cobcsw::fs
{
extern lfs_config const lfsConfig;

// TODO: Discuss appropriate error code
constexpr auto lockBusyError = -99;

auto Initialize() -> void;
}
