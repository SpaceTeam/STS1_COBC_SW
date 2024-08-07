#include <Sts1CobcSw/FileSystem/LfsWrapper.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>

#include <littlefs/lfs.h>

#include <iostream>


namespace sts1cobcsw::fs
{
lfs_t lfs{};


// FIXME: For some reason this allocates 1024 bytes on the heap. With LFS_NO_MALLOC defined, it
// crashes with a SEGFAULT.
auto Mount() -> Result<void>
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


auto Unmount() -> Result<void>
{
    auto error = lfs_unmount(&lfs);
    if(error != 0)
    {
        return static_cast<ErrorCode>(error);
    }
    return outcome_v2::success();
}


auto Open(std::string_view path, unsigned int flags) -> Result<File>
{
    auto file = File();
    auto error = lfs_file_opencfg(
        &lfs, &file.lfsFile_, path.data(), static_cast<int>(flags), &file.lfsFileConfig_);
    if(error == 0)
    {
        file.path_ = Path(path.data(), path.size());
        file.openFlags_ = flags;
        file.isOpen_ = true;
        return file;
    }
    return static_cast<ErrorCode>(error);
}

auto File::Move(File & other) noexcept -> void
{
    auto error = lfs_file_opencfg(
        &lfs, &lfsFile_, other.path_.c_str(), static_cast<int>(other.openFlags_), &lfsFileConfig_);
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

File::File(File && other) noexcept
{
    if(other.path_.empty())
    {
        return;
    }

    Move(other);
}


auto File::operator=(File && other) noexcept -> File &
{
    // TODO: Use copy and swap idiom to prevent code duplication from move constructor
    if(this != &other and not other.path_.empty())
    {
        Move(other);
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


auto File::Close() -> Result<void>
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
