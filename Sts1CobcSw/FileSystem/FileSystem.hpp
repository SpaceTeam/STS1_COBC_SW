#pragma once


#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>

#include <littlefs/lfs.h>

#include <etl/string.h>

#include <cstdint>


namespace sts1cobcsw::fs
{
using Path = etl::string<maxPathLength>;


[[nodiscard]] auto Mount() -> Result<void>;
[[nodiscard]] auto Unmount() -> Result<void>;
[[nodiscard]] auto CreateDirectory(Path const & path) -> Result<void>;
[[nodiscard]] auto Remove(Path const & path) -> Result<void>;
[[nodiscard]] auto ForceRemove(Path const & path) -> Result<void>;

[[nodiscard]] auto FileSize(Path const & path) -> Result<std::uint32_t>;
[[nodiscard]] auto IsLocked(Path const & path) -> Result<bool>;


namespace internal
{
extern lfs_t lfs;
}
}
