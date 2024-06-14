#pragma once

#include <Sts1CobcSw/FileSystem/LfsWrapper.hpp>

#include <littlefs/lfs.h>


namespace sts1cobcsw::fs
{
template<typename T>
auto File::Read(T * t) -> Result<int>
{
    // TODO: Also check openFlags_ for read permission
    if(not isOpen_)
    {
        return ErrorCode::fileNotOpen;
    }
    auto nReadBytes = lfs_file_read(&lfs, &lfsFile_, t, sizeof(T));
    if(nReadBytes >= 0)
    {
        return nReadBytes;
    }
    return static_cast<ErrorCode>(nReadBytes);
}


template<typename T>
auto File::Write(T const & t) -> Result<int>
{
    // TODO: Also check openFlags_ for write permission
    if(not isOpen_)
    {
        return ErrorCode::fileNotOpen;
    }
    auto nWrittenBytes = lfs_file_write(&lfs, &lfsFile_, &t, sizeof(T));
    if(nWrittenBytes >= 0)
    {
        return nWrittenBytes;
    }
    return static_cast<ErrorCode>(nWrittenBytes);
}
}