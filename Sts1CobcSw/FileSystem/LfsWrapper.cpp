#include <Sts1CobcSw/FileSystem/LfsStorageDevice.hpp>
#include <Sts1CobcSw/FileSystem/LfsWrapper.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>

#include <littlefs/lfs.h>

#include <iostream>


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
    error = lfs_format(&lfs, &lfsConfig);
    if(error != 0)
    {
        return static_cast<ErrorCode>(error);
    }
    error = lfs_mount(&lfs, &lfsConfig);
    if(error == 0)
    {
        return outcome_v2::success();
    }
    return static_cast<ErrorCode>(error);
}


auto Open(std::string_view path, int flags) -> Result<File>
{
    auto file = File();
    // TODO: Use lfs_file_opencfg() instead
    auto error = lfs_file_open(&lfs, &file.lfsFile_, path.data(), flags);
    if(error == 0)
    {
        file.path_ = Path(path.data(), path.size());
        file.openFlags_ = flags;
        file.isOpen_ = true;
        return file;
    }
    return static_cast<ErrorCode>(error);
}


File::File(File && other) noexcept
{
    if(other.path_.empty())
    {
        return;
    }
    // TODO: Use lfs_file_opencfg() instead
    auto error = lfs_file_open(&lfs, &lfsFile_, other.path_.c_str(), other.openFlags_);
    if(error == 0)
    {
        path_ = other.path_;
        openFlags_ = other.openFlags_;
        isOpen_ = true;
    }
    (void)other.Close();
    other.path_ = "";
    other.openFlags_ = 0;
    other.lfsFile_ = {};
}


auto File::operator=(File && other) noexcept -> File &
{
    // TODO: Use copy and swap idiom to prevent code duplication
    if(this != &other and not other.path_.empty())
    {
        // TODO: Use lfs_file_opencfg() instead
        auto error = lfs_file_open(&lfs, &lfsFile_, other.path_.c_str(), other.openFlags_);
        if(error == 0)
        {
            path_ = other.path_;
            openFlags_ = other.openFlags_;
            isOpen_ = true;
        }
        (void)other.Close();
        other.path_ = "";
        other.openFlags_ = 0;
        other.lfsFile_ = {};
    }
    return *this;
}


File::~File()
{
    // Only close the file if it is not in a default initialized or moved-from state
    if(not path_.empty())
    {
        auto closeResult = Close();
        if(closeResult.has_error())
        {
            std::cout << "Error closing file\n";
        }
    }
}


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


auto File::Size() -> Result<int>
{
    if(not isOpen_)
    {
        return ErrorCode::fileNotOpen;
    }
    auto size = lfs_file_size(&lfs, &lfsFile_);
    if(size >= 0)
    {
        return size;
    }
    return static_cast<ErrorCode>(size);
}


[[nodiscard]] auto File::Close() -> Result<void>
{
    if(not isOpen_)
    {
        return outcome_v2::success();
    }
    auto error = lfs_file_close(&lfs, &lfsFile_);
    if(error != 0)
    {
        return static_cast<ErrorCode>(error);
    }
    isOpen_ = false;
    return outcome_v2::success();
}
}
