//! @file
//! @brief  Implement a flash memory device for littlefs.

#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>  // IWYU pragma: associated
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <rodos/api/rodos-semaphore.h>
#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <array>
#include <span>


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
auto Lock(lfs_config const * config) -> int;
auto Unlock(lfs_config const * config) -> int;


// TODO: Test with real HW
// max. 3.5 ms acc. W25Q01JV datasheet
constexpr auto pageProgramTimeout = 5 * ms;
// max. 400 ms acc. W25Q01JV datasheet (lfs_config.block_size = flash::sectorSize)
constexpr auto blockEraseTimeout = 500 * ms;

auto readBuffer = std::array<Byte, lfsCacheSize>{};
auto programBuffer = decltype(readBuffer){};
auto lookaheadBuffer = std::array<Byte, 64>{};  // NOLINT(*magic-numbers)

// littlefs requires the lookaheadBuffer size to be a multiple of 8
static_assert(lookaheadBuffer.size() % 8 == 0);  // NOLINT(*magic-numbers)
// littlefs requires the cacheSize to be a multiple of the read_size and prog_size, i.e., pageSize
static_assert(lfsCacheSize % flash::pageSize == 0);

lfs_config const lfsConfig = lfs_config{.context = nullptr,
                                        .read = &Read,
                                        .prog = &Program,
                                        .erase = &Erase,
                                        .sync = &Sync,
                                        .lock = &Lock,
                                        .unlock = &Unlock,
                                        .read_size = flash::pageSize,
                                        .prog_size = flash::pageSize,
                                        .block_size = flash::sectorSize,
                                        .block_count = flash::nSectors,
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
                                        .metadata_max = flash::sectorSize,
                                        .inline_max = 0};

auto semaphore = RODOS::Semaphore();


auto Initialize() -> void
{
    flash::Initialize();
}


auto Read(lfs_config const * config,
          lfs_block_t blockNo,
          lfs_off_t offset,
          void * buffer,
          lfs_size_t size) -> int
{
    // The following only works if read_size == pageSize
    auto startAddress = blockNo * config->block_size + offset;
    for(auto i = 0U; i < size; i += config->read_size)
    {
        auto page = flash::ReadPage(startAddress + i);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::copy(page.begin(), page.end(), (static_cast<Byte *>(buffer) + i));
    }
    return 0;
}

auto Program(lfs_config const * config,
             lfs_block_t blockNo,
             lfs_off_t offset,
             void const * buffer,
             lfs_size_t size) -> int
{
    // The following only works if prog_size == pageSize
    auto startAddress = blockNo * config->block_size + offset;
    for(auto i = 0U; i < size; i += config->prog_size)
    {
        auto page = flash::Page{};
        std::copy((static_cast<Byte const *>(buffer) + i),                      // NOLINT
                  (static_cast<Byte const *>(buffer) + i + config->prog_size),  // NOLINT
                  page.begin());
        flash::ProgramPage(startAddress + i, std::span(page));
        auto waitWhileBusyResult = flash::WaitWhileBusy(pageProgramTimeout);
        if(waitWhileBusyResult.has_error())
        {
            return LFS_ERR_IO;
        }
    }
    return 0;
}


auto Erase(lfs_config const * config, lfs_block_t blockNo) -> int
{
    flash::EraseSector(blockNo * config->block_size);
    auto waitWhileBusyResult = flash::WaitWhileBusy(blockEraseTimeout);
    if(waitWhileBusyResult.has_error())
    {
        return LFS_ERR_IO;
    }
    return 0;
}


auto Sync([[maybe_unused]] lfs_config const * config) -> int
{
    auto waitWhileBusyResult = flash::WaitWhileBusy(pageProgramTimeout);
    if(waitWhileBusyResult.has_error())
    {
        return LFS_ERR_IO;
    }
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
