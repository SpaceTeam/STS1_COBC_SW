#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <array>
#include <span>


namespace sts1cobcsw::fs
{
using flash::pageSize;
using serial::Byte;


// --- Private function declarations ---

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


// --- Globals ---

auto readBuffer = std::array<serial::Byte, pageSize>{};
auto programBuffer = std::array<serial::Byte, pageSize>{};
auto lookaheadBuffer = std::array<serial::Byte, pageSize>{};

// TODO: Check if they need to be global
lfs_t lfs{};
lfs_file_t lfsFile{};
// TODO: Maybe add a conifg header to set things like NAME_MAX or whatever. That could safe a bit of
// RAM.
lfs_config const lfsConfig{.read = &Read,
                           .prog = &Program,
                           .erase = &Erase,
                           .sync = &Sync,

                           .read_size = pageSize,
                           .prog_size = pageSize,
                           .block_size = flash::sectorSize,
                           .block_count = flash::nSectors,
                           .block_cycles = 200,
                           .cache_size = pageSize,
                           .lookahead_size = pageSize,

                           .read_buffer = data(readBuffer),
                           .prog_buffer = data(programBuffer),
                           .lookahead_buffer = data(lookaheadBuffer)};  // NOLINT


// --- Public function definitions ---

auto Initialize() -> void
{
    [[maybe_unused]] auto errorCode = flash::Initialize();
}


auto Format() -> int
{
    return lfs_format(&lfs, &lfsConfig);
}


// Must be called before using the file system
auto Mount() -> int
{
    return lfs_mount(&lfs, &lfsConfig);
}


// TODO: This begs for a destructor
// Must be called to release all the resources of the file system
auto Unmount() -> int
{
    return lfs_unmount(&lfs);
}


auto OpenFile(char const * path, int flags) -> int
{
    return lfs_file_open(&lfs, &lfsFile, path, flags);
}


auto CloseFile() -> int
{
    return lfs_file_close(&lfs, &lfsFile);
}


//! @brief  Return the size of the currently open file.
auto FileSize() -> int
{
    return lfs_file_size(&lfs, &lfsFile);
}


auto CreateDirectory(char const * path) -> int
{
    return lfs_mkdir(&lfs, path);
}


//! @brief  Remove a file or an empty directory.
auto Remove(char const * path) -> int
{
    return lfs_remove(&lfs, path);
}


//! @brief  List information (type, size, name) about the files under the given path.
auto Ls(char const * path) -> int
{
    using RODOS::PRINTF;

    PRINTF("$ ls %s\n", path);

    auto directory = lfs_dir_t{};
    int const errorCode = lfs_dir_open(&lfs, &directory, path);
    if(errorCode != 0)
    {
        return errorCode;
    }

    auto info = lfs_info{};
    while(true)
    {
        int const result = lfs_dir_read(&lfs, &directory, &info);
        if(result < 0)
        {
            // An error occurred
            return result;
        }
        if(result == 0)
        {
            // We are at the end of the directory
            break;
        }

        switch(info.type)
        {
            case LFS_TYPE_REG:
            {
                PRINTF("  reg ");
                break;
            }
            case LFS_TYPE_DIR:
            {
                PRINTF("  dir ");
                break;
            }
            default:
            {
                PRINTF("  ?   ");
                break;
            }
        }

        PRINTF(" %8d B  %s\n", static_cast<int>(info.size), &(info.name[0]));
    }

    return lfs_dir_close(&lfs, &directory);
}


// --- Private function definitions ---

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
    // The following only works if prog_size == pageSize
    auto startAddress = blockNo * config->block_size + offset;
    for(auto i = 0U; i < size; i += config->prog_size)
    {
        auto page = flash::Page{};
        std::copy((static_cast<Byte const *>(buffer) + i),                      // NOLINT
                  (static_cast<Byte const *>(buffer) + i + config->prog_size),  // NOLINT
                  begin(page));
        flash::ProgramPage(startAddress + i, std::span(page));
        flash::WaitWhileBusy();
    }
    return 0;
}


auto Erase(lfs_config const * config, lfs_block_t blockNo) -> int
{
    flash::EraseSector(blockNo * config->block_size);
    flash::WaitWhileBusy();
    return 0;
}


auto Sync([[maybe_unused]] lfs_config const * config) -> int
{
    flash::WaitWhileBusy();
    return 0;
}
}
