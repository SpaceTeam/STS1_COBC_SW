#pragma once


#include <Sts1CobcSw/ProgramId/ProgramId.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <littlefs/lfs.h>

#include <etl/vector.h>

#include <cstddef>
#include <cstdint>


namespace sts1cobcsw::fs::deprecated
{
extern lfs_t lfs;
extern lfs_file_t lfsFile;
extern const lfs_config lfsConfig;


// Must be called once in a thread's init() function
auto Initialize() -> void;
auto Format() -> int;
auto Mount() -> int;
auto Unmount() -> int;

// File stuff
auto OpenProgramFile(ProgramId programId, int flags) -> int;
auto CloseProgramFile() -> int;
template<std::size_t size>
auto ReadProgramFile(etl::vector<Byte, size> * buffer) -> int;

auto OpenFile(char const * path, int flags) -> int;
auto CloseFile() -> int;
auto FileSize() -> int;
template<typename T>
auto ReadFromFile(T * t) -> int;
template<typename T>
auto WriteToFile(T const & t) -> int;

// Directory stuff
auto CreateDirectory(char const * path) -> int;

// Other stuff
auto Remove(char const * path) -> int;
auto Ls(char const * path) -> int;
// TODO: Implement cat
// TODO: Implement simple hexdump
}


#include <Sts1CobcSw/FileSystem/FileSystem.ipp>  // IWYU pragma: keep
