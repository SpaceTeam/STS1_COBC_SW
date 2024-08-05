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

auto Swap(File & lhs, File & rhs) -> void
{
    if(&lhs == &rhs)
    {
        return;
    }

    using std::swap;
    if(lhs.isOpen_)
    {
        auto error = lfs_file_close(&lfs, &lhs.lfsFile_);
        if(error != 0)
        {
            lhs.isOpen_ = false;  // close?
        }
    }
    if(rhs.isOpen_)
    {
        auto error = lfs_file_close(&lfs, &rhs.lfsFile_);
        if(error != 0)
        {
            rhs.isOpen_ = false;  // close?
        }
    }

    swap(lhs.path_, rhs.path_);
    swap(lhs.isOpen_, rhs.isOpen_);
    swap(lhs.buffer_, rhs.buffer_);
    swap(lhs.openFlags_, rhs.openFlags_);

    if(lhs.isOpen_ && not lhs.path_.empty())
    {
        auto error = lfs_file_opencfg(&lfs,
                                      &lhs.lfsFile_,
                                      lhs.path_.data(),
                                      static_cast<int>(lhs.openFlags_),
                                      &lhs.lfsFileConfig_);
        if(error != 0)
        {
            lhs.isOpen_ = false;  // close?
        }
    }
    if(rhs.isOpen_ && not rhs.path_.empty())
    {
        auto error = lfs_file_opencfg(&lfs,
                                      &rhs.lfsFile_,
                                      rhs.path_.data(),
                                      static_cast<int>(rhs.openFlags_),
                                      &rhs.lfsFileConfig_);
        if(error != 0)
        {
            lhs.isOpen_ = false;  // close?
        }
    }
}

File::File(File && other) noexcept
{
    if(other.path_.empty())
    {
        return;
    }

    Swap(*this, other);
    (void)other.Close();
}


auto File::operator=(File && other) noexcept -> File &
{
    if(this != &other and not other.path_.empty())
    {
        auto temporary = File(std::move(other));
        Swap(*this, temporary);
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
