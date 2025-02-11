//! @file
//! @brief  Implement a flash memory device for littlefs.

#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>  // IWYU pragma: associated
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/RodosTime.hpp>

#include <strong_type/difference.hpp>

#include <littlefs/lfs_util.h>

#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <array>
#include <cstring>
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

constexpr auto pageSizeLfs = flash::pageSize - crcSize;
constexpr auto sectorSizeLfs = flash::sectorSize - (flash::sectorSize / flash::pageSize * crcSize);

auto readBuffer = std::array<Byte, lfsCacheSize>{};
auto programBuffer = decltype(readBuffer){};
auto lookaheadBuffer = std::array<Byte, 64>{};  // NOLINT(*magic-numbers)

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
                                        .metadata_max = sectorSizeLfs,
                                        .inline_max = 0};

auto semaphore = RODOS::Semaphore();


auto Initialize() -> void
{
    flash::Initialize();
}


auto Read(lfs_config const * config,
          lfs_block_t blockNo,  // NOLINT(bugprone-easily-swappable-parameters)
          lfs_off_t offset,
          void * buffer,
          lfs_size_t size) -> int
{
    // The following only works if read_size == pageSize
    for(auto i = 0U; i < size; i += config->read_size)
    {
        auto const pages = (i + offset) / config->read_size;
        auto const offsetCrc = pages * flash::pageSize;
        auto const startAddress =
            static_cast<std::uint32_t>(blockNo * flash::sectorSize + offsetCrc);

        auto const page = flash::ReadPage(startAddress);

        // if the page is filled with eraseValue -> page was just erased and the crc is not set
        if(!std::all_of(page.begin(), page.end(), [](Byte byte) { return byte == eraseValue; }))
        {
            std::uint32_t crc = {};
            std::memcpy(&crc, page.begin() + config->read_size, sizeof(crc));

            const std::uint32_t crcCalculated =
                lfs_crc(initialCrcValue, page.begin(), config->read_size);
            if(crc != crcCalculated)
            {
                return LFS_ERR_CORRUPT;
            }
        }

        std::copy_n(page.begin(),
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
    // The following only works if prog_size == pageSize
    for(auto i = 0U; i < size; i += config->prog_size)
    {
        auto const pages = (i + offset) / config->prog_size;
        auto const offsetCrc = pages * flash::pageSize;
        auto const startAddress =
            static_cast<std::uint32_t>(blockNo * flash::sectorSize + offsetCrc);

        auto page = flash::Page{};
        std::copy_n(static_cast<Byte const *>(buffer) + i,  // NOLINT(*pointer-arithmetic)
                    config->prog_size,
                    page.begin());

        const std::uint32_t crc =
            lfs_crc(initialCrcValue,
                    static_cast<Byte const *>(buffer) + i,  // NOLINT(*pointer-arithmetic)
                    config->prog_size);
        std::memcpy(page.begin() + config->prog_size, &crc, sizeof(crc));

        flash::ProgramPage(startAddress, std::span(page));
        auto waitWhileBusyResult = flash::WaitWhileBusy(pageProgramTimeout);
        if(waitWhileBusyResult.has_error())
        {
            return LFS_ERR_IO;
        }
    }
    return 0;
}


auto Erase([[maybe_unused]] lfs_config const * config, lfs_block_t blockNo) -> int
{
    flash::EraseSector(blockNo * flash::sectorSize);
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
