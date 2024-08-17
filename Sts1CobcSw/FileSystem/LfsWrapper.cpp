#include <Sts1CobcSw/FileSystem/LfsWrapper.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>

#include <littlefs/lfs.h>


namespace sts1cobcsw::fs
{
auto lfs = lfs_t{};


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


File::File(File && other) noexcept
{
    MoveConstructFrom(&other);
}


auto File::operator=(File && other) noexcept -> File &
{
    if(this != &other)
    {
        MoveConstructFrom(&other);
    }
    return *this;
}


File::~File()
{
    // Only close the file if it is not in a default initialized or moved-from state
    if(not path_.empty())
    {
        (void)Close();
    }
}


auto File::Size() const -> Result<int>
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


// This function handles the weird way in which lfs_file_t objects are moved. It is more efficient
// than using the usual copy-and-swap idiom, because the latter would require to open and close the
// file more often.
auto File::MoveConstructFrom(File * other) noexcept -> void
{
    if(other->path_.empty())
    {
        return;
    }
    auto error = lfs_file_opencfg(&lfs,
                                  &lfsFile_,
                                  other->path_.c_str(),
                                  static_cast<int>(other->openFlags_),
                                  &lfsFileConfig_);
    if(error == 0)
    {
        path_ = other->path_;
        openFlags_ = other->openFlags_;
        isOpen_ = true;
    }
    (void)other->Close();
    other->path_ = "";
    other->openFlags_ = 0;
    other->lfsFile_ = {};
}


auto File::Read(void * buffer, std::size_t size) const -> Result<int>
{
    if(not isOpen_)
    {
        return ErrorCode::fileNotOpen;
    }
    if((openFlags_ & LFS_O_RDONLY) == 0U)
    {
        return ErrorCode::unsupportedOperation;
    }
    auto nReadBytes = lfs_file_read(&lfs, &lfsFile_, buffer, size);
    if(nReadBytes >= 0)
    {
        return nReadBytes;
    }
    return static_cast<ErrorCode>(nReadBytes);
}


auto File::Write(void const * buffer, std::size_t size) -> Result<int>
{
    if(not isOpen_)
    {
        return ErrorCode::fileNotOpen;
    }
    if((openFlags_ & LFS_O_WRONLY) == 0U)
    {
        return ErrorCode::unsupportedOperation;
    }
    auto nWrittenBytes = lfs_file_write(&lfs, &lfsFile_, buffer, size);
    if(nWrittenBytes >= 0)
    {
        return nWrittenBytes;
    }
    return static_cast<ErrorCode>(nWrittenBytes);
}
}
