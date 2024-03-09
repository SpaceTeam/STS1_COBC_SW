#pragma once


#include <littlefs/lfs.h>


namespace sts1cobcsw::fs
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
