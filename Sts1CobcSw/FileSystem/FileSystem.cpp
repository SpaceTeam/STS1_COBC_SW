#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <array>
#include <span>


namespace sts1cobcsw::fs
{
using periphery::flash::pageSize;
using RODOS::PRINTF;
using serial::Byte;


// --- Private function declarations

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


// --- Globals ---

auto readBuffer = std::array<serial::Byte, pageSize>{};
auto programBuffer = std::array<serial::Byte, pageSize>{};
auto lookaheadBuffer = std::array<serial::Byte, pageSize>{};

// TODO: Check if they need to be global
lfs_t lfs{};
lfs_file_t lfsFile{};
const lfs_config lfsConfig{.read = &Read,
                           .prog = &Program,
                           .erase = &Erase,
                           .sync = &Sync,

                           .read_size = pageSize,
                           .prog_size = pageSize,
                           .block_size = periphery::flash::sectorSize,
                           .block_count = periphery::flash::nSectors,
                           .block_cycles = 500,
                           .cache_size = pageSize,
                           .lookahead_size = pageSize,

                           .read_buffer = data(readBuffer),
                           .prog_buffer = data(programBuffer),
                           .lookahead_buffer = data(lookaheadBuffer)};


// --- Public function definitions

auto Initialize() -> void
{
    [[maybe_unused]] auto errorCode = periphery::flash::Initialize();
}

auto Mount() -> void
{
    PRINTF("Mounting...\n");
    int errorCode = lfs_mount(&lfs, &lfsConfig);
    PRINTF("Returned error code = %d\n\n", errorCode);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if(errorCode != 0)
    {
        PRINTF("Formatting...\n");
        errorCode = lfs_format(&lfs, &lfsConfig);
        PRINTF("Returned error code = %d\n\n", errorCode);

        PRINTF("Mounting...\n");
        errorCode = lfs_mount(&lfs, &lfsConfig);
        PRINTF("Returned error code = %d\n\n", errorCode);
    }
}


// TODO: This begs for a destructor
auto Unmount() -> void
{
    PRINTF("Unmounting...\n");
    auto errorCode = lfs_unmount(&lfs);
    PRINTF("Returned error code = %d\n\n", errorCode);
}


// --- Private function definitions

auto Read(lfs_config const * config,
          lfs_block_t blockNo,
          lfs_off_t offset,
          void * buffer,
          lfs_size_t size) -> int
{
    PRINTF("Read(blockNo=%d, offset=%d, size=%d)\n",
           static_cast<int>(blockNo),
           static_cast<int>(offset),
           static_cast<int>(size));

    // The following only works if read_size == pageSize
    auto startAddress = blockNo * config->block_size + offset;
    for(auto i = 0U; i < size; i += config->read_size)
    {
        auto page = periphery::flash::ReadPage(startAddress + i);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::copy(begin(page), end(page), (static_cast<Byte *>(buffer) + i));
    }

    return 0;
}


auto Program(lfs_config const * config,
             lfs_block_t blockNo,
             lfs_off_t offset,
             void const * buffer,
             lfs_size_t size) -> int
{
    PRINTF("Program(blockNo=%d, offset=%d, size=%d)\n",
           static_cast<int>(blockNo),
           static_cast<int>(offset),
           static_cast<int>(size));

    // The following only works if prog_size == pageSize
    auto startAddress = blockNo * config->block_size + offset;
    for(auto i = 0U; i < size; i += config->prog_size)
    {
        auto page = periphery::flash::Page{};
        std::copy((static_cast<Byte const *>(buffer) + i),                      // NOLINT
                  (static_cast<Byte const *>(buffer) + i + config->prog_size),  // NOLINT
                  begin(page));
        periphery::flash::ProgramPage(startAddress + i, std::span(page));
        periphery::flash::WaitWhileBusy();
    }
    return 0;
}


auto Erase(lfs_config const * config, lfs_block_t blockNo) -> int
{
    PRINTF("Erase(blockNo=%d)\n", static_cast<int>(blockNo));
    periphery::flash::EraseSector(blockNo * config->block_size);
    periphery::flash::WaitWhileBusy();
    return 0;
}


auto Sync([[maybe_unused]] lfs_config const * config) -> int
{
    PRINTF("Sync()\n");
    periphery::flash::WaitWhileBusy();
    return 0;
}
}