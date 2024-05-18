//! @file
//! @brief Simulate a storage device for littlefs in RAM.
//!
//! This is useful for testing the file system without using a real flash memory.

#include <Sts1CobcSw/FileSystem/LfsStorageDevice.hpp>  // IWYU pragma: associated
#include <Sts1CobcSw/Serial/Byte.hpp>

#ifdef NO_RODOS
    #include <mutex>
#else
    #include <Sts1CobcSw/Utility/LinuxSemaphore.hpp>

    #include <rodos_no_using_namespace.h>
#endif

#include <algorithm>
#include <array>
#include <vector>


namespace sts1cobcsw::fs
{
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
// TODO: Implement Lock() and Unlock()
auto Lock(const struct lfs_config * config) -> int;
auto Unlock(const struct lfs_config * config) -> int;


constexpr auto pageSize = 256;
constexpr auto sectorSize = 4 * 1024;
constexpr auto memorySize = 128 * 1024 * 1024;

auto memory = std::vector<Byte>();
auto readBuffer = std::array<Byte, pageSize>{};
auto programBuffer = decltype(readBuffer){};
auto lookaheadBuffer = std::array<Byte, pageSize>{};

#ifdef NO_RODOS
auto mutex = std::mutex();
#else
auto mutex = utility::LinuxSemaphore();
#endif

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
                                        .read_buffer = readBuffer.data(),
                                        .prog_buffer = programBuffer.data(),
                                        .lookahead_buffer = lookaheadBuffer.data(),
                                        .name_max = LFS_NAME_MAX,
                                        .file_max = LFS_FILE_MAX,
                                        .attr_max = LFS_ATTR_MAX,
                                        .metadata_max = sectorSize};


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


auto Lock([[maybe_unused]] lfs_config const * config) -> int
{
    static constexpr auto mutexUnavailableError = -99;
    // TODO: Check if there is an appropriate error code
#ifdef NO_RODOS
    if(mutex.try_lock())
    {
        return 0;
    }
    return mutexUnavailableError;
#else
    if(mutex.TryEnter())
    {
        // For testing: wait some time to check concurrent locking
        static constexpr auto lockDelay = 200 * RODOS::MILLISECONDS;
        RODOS::AT(RODOS::NOW() + lockDelay);
        return 0;
    }
    return mutexUnavailableError;
#endif
}


auto Unlock([[maybe_unused]] lfs_config const * config) -> int
{
#ifdef NO_RODOS
    mutex.unlock();
    return 0;
#else
    mutex.Leave();
    return 0;
#endif
}
}
