#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>

#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <iterator>
#include <span>


namespace sts1cobcsw::fs::deprecated
{
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

auto readBuffer = flash::Page{};
auto programBuffer = flash::Page{};
auto lookaheadBuffer = flash::Page{};

lfs_t lfs{};
lfs_file_t lfsFile{};
// TODO: Maybe add a config header to set things like NAME_MAX or whatever. That could safe a bit of
// RAM.
lfs_config const lfsConfig{.context = nullptr,

                           .read = &Read,
                           .prog = &Program,
                           .erase = &Erase,
                           .sync = &Sync,

                           .read_size = flash::pageSize,
                           .prog_size = flash::pageSize,
                           .block_size = flash::sectorSize,
                           .block_count = flash::nSectors,
                           .block_cycles = 200,
                           .cache_size = flash::pageSize,
                           .lookahead_size = flash::pageSize,

                           .read_buffer = data(readBuffer),
                           .prog_buffer = data(programBuffer),
                           .lookahead_buffer = data(lookaheadBuffer),

                           .name_max = LFS_NAME_MAX,
                           .file_max = LFS_FILE_MAX,
                           .attr_max = LFS_ATTR_MAX,
                           .metadata_max = flash::sectorSize};

// TODO: Test with real HW
// max. 3.5 ms acc. W25Q01JV datasheet
constexpr auto pageProgramTimeout = 5 * ms;
// max. 400 ms acc. W25Q01JV datasheet (lfs_config.block_size = flash::sectorSize)
constexpr auto blockEraseTimeout = 500 * ms;


// --- Public function definitions ---

auto Initialize() -> void
{
    flash::Initialize();
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


auto OpenProgramFile(ProgramId programId, int flags) -> int
{
    return 0;
}


auto CloseProgramFile() -> int
{
    return 0;
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
}
