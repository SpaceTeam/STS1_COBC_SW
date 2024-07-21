//! @file
//! @brief Simulate a storage device for littlefs in RAM.
//!
//! This is useful for testing the file system without using a real flash memory.

#include <Sts1CobcSw/FileSystem/LfsStorageDevice.hpp>  // IWYU pragma: associated
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <algorithm>
#include <array>
#include <vector>


namespace sts1cobcsw::fs
{
// Before globals because lfsConfig needs the declarations
auto Read(lfs_config const * config,
          lfs_block_t blockNo,
          lfs_off_t offset,
          void * buffer,
          lfs_size_t size) -> int;
auto Program(lfs_config const * config,
             lfs_block_t blockNo,
             lfs_off_t offset,
             void const * buffer,
             lfs_size_t size) -> int;
auto Erase(lfs_config const * config, lfs_block_t blockNo) -> int;
auto Sync(lfs_config const * config) -> int;
auto Lock(lfs_config const * config) -> int;
auto Unlock(lfs_config const * config) -> int;


constexpr auto pageSize = 256;
constexpr auto sectorSize = 4 * 1024;
constexpr auto memorySize = 128 * 1024 * 1024;

auto memory = std::vector<Byte>();
auto readBuffer = std::array<Byte, pageSize>{};
auto programBuffer = decltype(readBuffer){};
auto lookaheadBuffer = std::array<Byte, pageSize>{};

lfs_config const lfsConfig = lfs_config{.context = nullptr,
                                        .read = &Read,
                                        .prog = &Program,
                                        .erase = &Erase,
                                        .sync = &Sync,
                                        .lock = &Lock,
                                        .unlock = &Unlock,
                                        .read_size = pageSize,
                                        .prog_size = pageSize,
                                        .block_size = sectorSize,
                                        .block_count = memorySize / sectorSize,
                                        .block_cycles = 200,
                                        .cache_size = readBuffer.size(),
                                        .lookahead_size = lookaheadBuffer.size(),
                                        .compact_thresh = 0,
                                        .read_buffer = readBuffer.data(),
                                        .prog_buffer = programBuffer.data(),
                                        .lookahead_buffer = lookaheadBuffer.data(),
                                        .name_max = LFS_NAME_MAX,
                                        .file_max = LFS_FILE_MAX,
                                        .attr_max = LFS_ATTR_MAX,
                                        .metadata_max = sectorSize,
                                        .inline_max = 0};


auto Initialize() -> void
{
    memory.resize(memorySize, 0xFF_b);  // NOLINT(*magic-numbers*)
}


auto Read(lfs_config const * config,
          lfs_block_t blockNo,
          lfs_off_t offset,
          void * buffer,
          lfs_size_t size) -> int
{
    auto start = static_cast<int>(blockNo * config->block_size + offset);
    std::copy_n(memory.begin() + start, size, static_cast<Byte *>(buffer));
    return 0;
}

auto Program(lfs_config const * config,
             lfs_block_t blockNo,
             lfs_off_t offset,
             void const * buffer,
             lfs_size_t size) -> int
{
    auto start = static_cast<int>(blockNo * config->block_size + offset);
    std::copy_n(static_cast<Byte const *>(buffer), size, memory.begin() + start);
    return 0;
}


auto Erase(lfs_config const * config, lfs_block_t blockNo) -> int
{
    auto start = static_cast<int>(blockNo * config->block_size);
    std::fill_n(memory.begin() + start, config->block_size, 0xFF_b);  // NOLINT(*magic-numbers*)
    return 0;
}


auto Sync([[maybe_unused]] lfs_config const * config) -> int
{
    return 0;
}


// TODO: Add a proper implementation
auto Lock([[maybe_unused]] lfs_config const * config) -> int
{
    return 0;
}


// TODO: Add a proper implementation
auto Unlock([[maybe_unused]] lfs_config const * config) -> int
{
    return 0;
}
}
