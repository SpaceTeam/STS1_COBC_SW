#pragma once


#include <Sts1CobcSw/Filesystem/FileSystem.hpp>


namespace sts1cobcsw::fs
{
template<typename T>
inline auto ReadFromFile(T * t) -> int
{
    return lfs_file_read(&lfs, &lfsFile, t, sizeof(T));
}


template<typename T>
inline auto WriteToFile(T const & t) -> int
{
    return lfs_file_write(&lfs, &lfsFile, &t, sizeof(T));
}
}
