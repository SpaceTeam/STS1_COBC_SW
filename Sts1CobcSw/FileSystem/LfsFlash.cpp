//! @file
//! @brief  Implement a flash storage device for littlefs.

#include <Sts1CobcSw/FileSystem/LfsStorageDevice.hpp>  // IWYU pragma: associated
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <algorithm>
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


auto readBuffer = flash::Page{};
auto programBuffer = decltype(readBuffer){};
auto lookaheadBuffer = flash::Page{};

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
                                        .name_max = LFS_NAME_MAX,
                                        .file_max = LFS_FILE_MAX,
                                        .attr_max = LFS_ATTR_MAX,
                                        .metadata_max = flash::sectorSize,
                                        .inline_max = 0};

// TODO: Test with real HW
// max. 3.5 ms acc. W25Q01JV datasheet
constexpr auto pageProgramTimeout = 5 * RODOS::MILLISECONDS;
// max. 400 ms acc. W25Q01JV datasheet (lfs_config.block_size = flash::sectorSize)
constexpr auto blockEraseTimeout = 500 * RODOS::MILLISECONDS;


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
