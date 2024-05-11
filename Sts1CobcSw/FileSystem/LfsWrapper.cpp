#include <Sts1CobcSw/FileSystem/LfsStorageDevice.hpp>
#include <Sts1CobcSw/FileSystem/LfsWrapper.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>

#include <littlefs/lfs.h>


namespace sts1cobcsw::fs
{
auto lfs = lfs_t{};


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
    auto value = lfs_file_read(&lfs, &lfsFile_, t, sizeof(T));
    if(value >= 0)
    {
        return value;
    }
    return static_cast<ErrorCode>(value);
}


template<typename T>
auto File::Write(T const & t) -> Result<int>
{
    auto value = lfs_file_write(&lfs, &lfsFile_, &t, sizeof(T));
    if(value >= 0)
    {
        return value;
    }
    return static_cast<ErrorCode>(value);
}


auto File::Size() -> Result<int>
{
    auto value = lfs_file_size(&lfs, &lfsFile_);
    if(value >= 0)
    {
        return value;
    }
    return static_cast<ErrorCode>(value);
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


[[nodiscard]] auto Mount() -> Result<void>
{
    auto error = lfs_mount(&lfs, &lfsConfig);
    if(error == 0)
    {
        return outcome_v2::success();
    }
    return static_cast<ErrorCode>(error);
}
}
