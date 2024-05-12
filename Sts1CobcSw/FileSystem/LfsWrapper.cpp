#include <Sts1CobcSw/FileSystem/LfsStorageDevice.hpp>
#include <Sts1CobcSw/FileSystem/LfsWrapper.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>

#include <littlefs/lfs.h>


namespace sts1cobcsw::fs
{
auto lfs = lfs_t{};


[[nodiscard]] auto Mount() -> Result<void>
{
    auto error = lfs_mount(&lfs, &lfsConfig);
    if(error == 0)
    {
        return outcome_v2::success();
    }
    return static_cast<ErrorCode>(error);
}


auto Open(std::string_view path, int flag) -> Result<File>
{
    File file = File();
    auto error = lfs_file_open(&lfs, &file.lfsFile_, path.data(), flag);
    if(error == 0)
    {
        return file;
    }
    return static_cast<ErrorCode>(error);
}


File::~File()
{
    (void)Close();
}


template<typename T>
auto File::Read(T * t) -> Result<int>
{
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
    auto nWrittenBytes = lfs_file_write(&lfs, &lfsFile_, &t, sizeof(T));
    if(nWrittenBytes >= 0)
    {
        return nWrittenBytes;
    }
    return static_cast<ErrorCode>(nWrittenBytes);
}


auto File::Size() -> Result<int>
{
    auto size = lfs_file_size(&lfs, &lfsFile_);
    if(size >= 0)
    {
        return size;
    }
    return static_cast<ErrorCode>(size);
}


[[nodiscard]] auto File::Close() -> Result<void>
{
    auto error = lfs_file_close(&lfs, &lfsFile_);
    if(error == 0)
    {
        return outcome_v2::success();
    }
    return static_cast<ErrorCode>(error);
}
}
