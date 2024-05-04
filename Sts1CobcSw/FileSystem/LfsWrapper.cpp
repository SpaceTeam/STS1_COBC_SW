#include <Sts1CobcSw/FileSystem/LfsStorageDevice.hpp>
#include <Sts1CobcSw/FileSystem/LfsWrapper.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>

#include <littlefs/lfs.h>


namespace sts1cobcsw::fs
{
auto lfs = lfs_t{};

auto open(char const * path, int flag) -> Result<File>
{
    lfs_file_t lfsFile;
    auto error = lfs_file_open(&lfs, &lfsFile, path, flag);
    if(error == 0)
    {
        return File(lfsFile);
    }
    return static_cast<ErrorCode>(error);
}

File::~File()
{
    (void)close();
}

File::File(lfs_file_t & lfsFile) : m_lfsFile(lfsFile)
{
}

template<typename T>
auto File::read(T * t) -> Result<int>
{
    auto value = lfs_file_read(&lfs, &m_lfsFile, t, sizeof(T));
    if(value >= 0)
    {
        return value;
    }
    return static_cast<ErrorCode>(value);
}

template<typename T>
auto File::write(T const & t) -> Result<int>
{
    auto value = lfs_file_write(&lfs, &m_lfsFile, &t, sizeof(T));
    if(value >= 0)
    {
        return value;
    }
    return static_cast<ErrorCode>(value);
}

auto File::FileSize() -> Result<int>
{
    auto value = lfs_file_size(&lfs, &m_lfsFile);
    if(value >= 0)
    {
        return value;
    }
    return static_cast<ErrorCode>(value);
}

[[nodiscard]] auto File::close() -> Result<void>
{
    auto error = lfs_file_close(&lfs, &m_lfsFile);
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
