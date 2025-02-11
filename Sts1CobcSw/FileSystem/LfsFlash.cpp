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
namespace
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

constexpr auto erasedValue = 0xFF_b;
constexpr auto initialCrcValue = 0;

constexpr auto readSize = flash::pageSize - crcSize;
constexpr auto blockSize = flash::sectorSize - (flash::sectorSize / flash::pageSize * crcSize);


auto readBuffer = std::array<Byte, lfsCacheSize>{};
auto programBuffer = decltype(readBuffer){};
auto lookaheadBuffer = std::array<Byte, 64>{};  // NOLINT(*magic-numbers)

auto semaphore = RODOS::Semaphore();
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
                                        .metadata_max = blockSize,
                                        .inline_max = 0};


auto Initialize() -> void
{
    flash::Initialize();
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
        auto pageAddress =
            static_cast<std::uint32_t>(blockNo * flash::sectorSize + pageNo * flash::pageSize);
        auto page = flash::ReadPage(pageAddress);
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
    for(auto i = 0U; i < size; i += config->prog_size)
    {
        auto page = flash::Page{};
        // NOLINTNEXTLINE(*pointer-arithmetic)
        std::copy_n(static_cast<Byte const *>(buffer) + i, config->prog_size, page.begin());
        auto crc = lfs_crc(initialCrcValue, page.data(), config->prog_size);
        static_assert(sizeof(crc) == crcSize);
        std::memcpy(&page[config->prog_size], &crc, sizeof(crc));

        auto pageNo = (i + offset) / config->prog_size;
        auto pageAddress =
            static_cast<std::uint32_t>(blockNo * flash::sectorSize + pageNo * flash::pageSize);
        flash::ProgramPage(pageAddress, std::span(page));
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
}
