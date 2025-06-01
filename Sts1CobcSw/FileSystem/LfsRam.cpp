//! @file
//! @brief Simulate a memory device for littlefs in RAM.
//!
//! This is useful for testing the file system without using a real flash memory.

#include <Sts1CobcSw/FileSystem/LfsRam.hpp>

#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>  // IWYU pragma: associated
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <littlefs/lfs.h>
#include <littlefs/lfs_util.h>

#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>


namespace sts1cobcsw::fs
{
namespace
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


constexpr auto erasedValue = 0xFF_b;
constexpr auto initialCrcValue = 0;

constexpr auto pageSize = 256;
constexpr auto readSize = pageSize - crcSize;
constexpr auto sectorSize = 4 * 1024;
constexpr auto blockSize = sectorSize * readSize / pageSize;
constexpr auto memorySize = 128 * 1024 * 1024;


auto readBuffer = std::array<Byte, lfsCacheSize>{};
auto programBuffer = decltype(readBuffer){};
auto lookaheadBuffer = std::array<Byte, 64>{};  // NOLINT(*magic-numbers)

auto semaphore = RODOS::Semaphore();
void (*programFinishedHandler)() = nullptr;
}

// littlefs requires the lookaheadBuffer size to be a multiple of 8
static_assert(lookaheadBuffer.size() % 8 == 0);  // NOLINT(*magic-numbers)
// littlefs requires the cacheSize to be a multiple of the read_size (and prog_size)
static_assert(lfsCacheSize % readSize == 0);

lfs_config const lfsConfig = lfs_config{.context = nullptr,
                                        .read = &Read,
                                        .prog = &Program,
                                        .erase = &Erase,
                                        .sync = &Sync,
                                        .lock = &Lock,
                                        .unlock = &Unlock,
                                        .read_size = readSize,
                                        .prog_size = readSize,
                                        .block_size = blockSize,
                                        .block_count = memorySize / sectorSize,
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
                                        .metadata_max = blockSize,
                                        .inline_max = 0};

std::vector<Byte> memory = std::vector<Byte>();


auto Initialize() -> void
{
    memory.resize(memorySize, erasedValue);
}


auto SetProgramFinishedHandler(void (*handler)()) -> void
{
    programFinishedHandler = handler;
}


namespace
{
auto Read(lfs_config const * config,
          lfs_block_t blockNo,  // NOLINT(bugprone-easily-swappable-parameters)
          lfs_off_t offset,
          void * buffer,
          lfs_size_t size) -> int
{
    // Read page for page and check the CRC of each one
    for(auto i = 0U; i < size; i += config->read_size)
    {
        auto pageNo = (i + offset) / config->read_size;
        auto pageAddress = blockNo * sectorSize + pageNo * pageSize;
        auto page = std::span(&memory[pageAddress], pageSize);
        auto pageIsErased =
            std::all_of(page.begin(), page.end(), [](auto byte) { return byte == erasedValue; });
        // Check the CRC only if the page is not erased
        if(not pageIsErased)
        {
            std::uint32_t crc;  // NOLINT(*init-variables)
            static_assert(sizeof(crc) == crcSize);
            std::memcpy(&crc, &page[config->read_size], sizeof(crc));
            auto computedCrc = lfs_crc(initialCrcValue, page.data(), config->read_size);
            if(crc != computedCrc)
            {
                return LFS_ERR_CORRUPT;
            }
        }
        // NOLINTNEXTLINE(*pointer-arithmetic)
        std::copy_n(page.begin(), config->read_size, static_cast<Byte *>(buffer) + i);
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
        auto pageNo = (i + offset) / config->prog_size;
        auto pageAddress = blockNo * sectorSize + pageNo * pageSize;
        auto page = std::span(&memory[pageAddress], pageSize);
        // NOLINTNEXTLINE(*pointer-arithmetic)
        std::copy_n(static_cast<Byte const *>(buffer) + i, config->prog_size, page.begin());
        auto crc = lfs_crc(initialCrcValue, page.data(), config->prog_size);
        static_assert(sizeof(crc) == crcSize);
        std::memcpy(&page[config->prog_size], &crc, sizeof(crc));
    }
    if(programFinishedHandler != nullptr)
    {
        programFinishedHandler();
    }
    return 0;
}


auto Erase([[maybe_unused]] lfs_config const * config, lfs_block_t blockNo) -> int
{
    std::fill_n(memory.begin() + static_cast<int>(blockNo * sectorSize), sectorSize, erasedValue);
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
}
}
