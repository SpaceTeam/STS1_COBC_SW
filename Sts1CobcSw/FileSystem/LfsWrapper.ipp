#pragma once

#include <Sts1CobcSw/FileSystem/LfsWrapper.hpp>

#include <littlefs/lfs.h>

#include "Sts1CobcSw/FileSystem/ErrorsAndResult.hpp"


namespace sts1cobcsw::fs
{
template<typename T>
auto File::Read(T * t) -> Result<int>
{
    if(not isOpen_)
    {
        return ErrorCode::fileNotOpen;
    }
    if(not(static_cast<uint>(openFlags_) & LFS_O_RDONLY))
    {
        return ErrorCode::invalidParameter;
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
    if(not isOpen_)
    {
        return ErrorCode::fileNotOpen;
    }
    if(not(static_cast<uint>(openFlags_) & LFS_O_WRONLY))
    {
        return ErrorCode::invalidParameter;
    }
    auto nWrittenBytes = lfs_file_write(&lfs, &lfsFile_, &t, sizeof(T));
    if(nWrittenBytes >= 0)
    {
        return nWrittenBytes;
    }
    return static_cast<ErrorCode>(nWrittenBytes);
}
}
