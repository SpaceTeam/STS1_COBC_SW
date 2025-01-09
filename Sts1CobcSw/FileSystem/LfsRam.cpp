//! @file
//! @brief Simulate a memory device for littlefs in RAM.
//!
//! This is useful for testing the file system without using a real flash memory.

#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>  // IWYU pragma: associated
#include <Sts1CobcSw/FileSystem/LfsRam.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <littlefs/lfs.h>
#include <littlefs/lfs_util.h>

#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
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
constexpr auto pageSizeLfs = pageSize - crcSize;
constexpr auto sectorSize = 4 * 1024;
constexpr auto sectorSizeLfs = 4 * (1024 - 4 * crcSize);
constexpr auto memorySizeLfs = 128 * 1024 * (1024 - 4 * crcSize);
constexpr auto memorySize = 128 * 1024 * 1024;

std::vector<Byte> memory = std::vector<Byte>();
auto readBuffer = std::array<Byte, lfsCacheSize>{};
auto programBuffer = decltype(readBuffer){};
auto lookaheadBuffer = std::array<Byte, 64>{};  // NOLINT(*magic-numbers)
auto corruptNextWrite = false;

// littlefs requires the lookaheadBuffer size to be a multiple of 8
static_assert(lookaheadBuffer.size() % 8 == 0);  // NOLINT(*magic-numbers)
// littlefs requires the cacheSize to be a multiple of the read_size and prog_size, i.e., pageSize
static_assert(lfsCacheSize % pageSizeLfs == 0);

lfs_config const lfsConfig = lfs_config{.context = nullptr,
                                        .read = &Read,
                                        .prog = &Program,
                                        .erase = &Erase,
                                        .sync = &Sync,
                                        .lock = &Lock,
                                        .unlock = &Unlock,
                                        .read_size = pageSizeLfs,
                                        .prog_size = pageSizeLfs,
                                        .block_size = sectorSizeLfs,
                                        .block_count = memorySizeLfs / sectorSizeLfs,
                                        .block_cycles = 200,
                                        .cache_size = readBuffer.size(),
                                        .lookahead_size = lookaheadBuffer.size(),
                                        .compact_thresh = 0,
                                        .read_buffer = readBuffer.data(),
                                        .prog_buffer = programBuffer.data(),
                                        .lookahead_buffer = lookaheadBuffer.data(),
                                        .name_max = maxPathLength,
                                        .file_max = LFS_FILE_MAX,
                                        .attr_max = LFS_ATTR_MAX,
                                        .metadata_max = sectorSizeLfs,
                                        .inline_max = 0};

auto semaphore = RODOS::Semaphore();
void (*programFinishedHandler)() = nullptr;


auto Initialize() -> void
{
    memory.resize(memorySize, eraseValue);
}


auto Read(lfs_config const * config,
          lfs_block_t blockNo,  // NOLINT(bugprone-easily-swappable-parameters)
          lfs_off_t offset,
          void * buffer,
          lfs_size_t size) -> int
{
    // read page for page to check the crc of each one
    for(lfs_size_t i = 0; i < size; i += config->read_size)
    {
        auto const pages = (i + offset) / config->read_size;
        auto const offsetCrc = pages * pageSize;
        auto const start = static_cast<size_t>(blockNo * sectorSize + offsetCrc);
        auto const crcPosition = start + config->read_size;

        // if the page is filled with eraseValue -> page was just erased and the crc is not set
        if(!std::all_of(memory.begin() + static_cast<int>(start),
                        memory.begin() + static_cast<int>(crcPosition),
                        [](Byte byte) { return byte == eraseValue; }))
        {
            std::uint32_t crc = {};
            std::memcpy(&crc, &memory[crcPosition], sizeof(crc));

            const std::uint32_t crcCalculated =
                lfs_crc(initialCrcValue, &memory[start], config->read_size);

            if(crc != crcCalculated)
            {
                return LFS_ERR_CORRUPT;
            }
        }

        std::copy_n(memory.begin() + static_cast<int>(start),
                    config->read_size,
                    static_cast<Byte *>(buffer) + i);  // NOLINT(*pointer-arithmetic)
    }
    return 0;
}


auto Program(lfs_config const * config,
             lfs_block_t blockNo,  // NOLINT(bugprone-easily-swappable-parameters)
             lfs_off_t offset,
             void const * buffer,
             lfs_size_t size) -> int
{
    for(lfs_size_t i = 0; i < size; i += config->prog_size)
    {
        auto const pages = (i + offset) / config->prog_size;
        auto const offsetCrc = pages * pageSize;
        auto const start = static_cast<size_t>(blockNo * sectorSize + offsetCrc);

        // copy data
        std::copy_n(static_cast<Byte const *>(buffer) + i,  // NOLINT(*pointer-arithmetic)
                    config->prog_size,
                    memory.begin() + static_cast<int>(start));
        const std::uint32_t crc =
            lfs_crc(initialCrcValue,
                    static_cast<Byte const *>(buffer) + i,  // NOLINT(*pointer-arithmetic)
                    config->prog_size);

        // copy crc
        const std::uint32_t crcPosition = start + config->prog_size;
        std::memcpy(&memory[crcPosition], &crc, sizeof(crc));
    }
    if(programFinishedHandler != nullptr)
    {
        programFinishedHandler();
    }
    return 0;
}


auto Erase([[maybe_unused]] lfs_config const * config, lfs_block_t blockNo) -> int
{
    auto start = static_cast<size_t>(blockNo * sectorSize);
    std::fill_n(memory.begin() + static_cast<int>(start), sectorSize, eraseValue);
    return 0;
}


auto Sync([[maybe_unused]] lfs_config const * config) -> int
{
    return 0;
}


auto Lock([[maybe_unused]] lfs_config const * config) -> int
{
    semaphore.enter();
    return 0;
}


auto Unlock([[maybe_unused]] lfs_config const * config) -> int
{
    semaphore.leave();
    return 0;
}


auto SetProgramFinishedHandler(void (*handler)()) -> void
{
    programFinishedHandler = handler;
}
}
