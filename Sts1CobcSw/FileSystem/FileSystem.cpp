#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <array>


namespace sts1cobcsw::fs
{
using periphery::flash::pageSize;
using RODOS::PRINTF;


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
    return 0;
}


auto Erase(lfs_config const * config, lfs_block_t blockNo) -> int
{
    PRINTF("Erase(blockNo=%d)\n", static_cast<int>(blockNo));
    return 0;
}


auto Sync(lfs_config const * config) -> int
{
    PRINTF("Sync()\n");
    return 0;
}
}