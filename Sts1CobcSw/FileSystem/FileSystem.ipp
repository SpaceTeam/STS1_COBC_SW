#pragma once


#include <Sts1CobcSw/FileSystem/FileSystem.hpp>


namespace sts1cobcsw::fs
{
template<std::size_t size>
auto ReadProgramFile(etl::vector<Byte, size> * buffer) -> int
{
    buffer->clear();
    for(int i = 1; i <= 10; ++i)  // NOLINT(*magic-numbers)
    {
        buffer->push_back(static_cast<Byte>(i));
    }
    return 0;
}


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
