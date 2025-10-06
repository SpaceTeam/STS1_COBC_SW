#include <Sts1CobcSw/FileSystem/File.hpp>

#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>


namespace sts1cobcsw::fs
{
namespace
{
auto & lfs = internal::lfs;
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


auto Open(Path const & path, unsigned int flags) -> Result<File>
{
    if(not persistentVariables.Load<"flashIsWorking">())
    {
        return ErrorCode::io;
    }
    auto file = File();
    // We need to create a temporary with Path(path) here since append() modifies the object and we
    // don't want to change the original path
    file.lockFilePath_ = BuildLockFilePath(path);
    auto createLockFileResult = file.CreateLockFile();
    if(createLockFileResult.has_error())
    {
        if(createLockFileResult.error() == ErrorCode::alreadyExists)
        {
            return ErrorCode::fileLocked;
        }
        return createLockFileResult.error();
    }
    auto error = lfs_file_opencfg(
        &lfs, &file.lfsFile_, path.c_str(), static_cast<int>(flags), &file.lfsFileConfig_);
    if(error == 0)
    {
        file.path_ = path;
        file.openFlags_ = flags;
        file.isOpen_ = true;
        return file;
    }
    return static_cast<ErrorCode>(error);
}


auto File::Resize(std::size_t newSize) -> Result<void>
{
    if(not persistentVariables.Load<"flashIsWorking">())
    {
        return ErrorCode::io;
    }
    if(not isOpen_)
    {
        return ErrorCode::fileNotOpen;
    }
    if((openFlags_ & LFS_O_WRONLY) == 0U)
    {
        return ErrorCode::unsupportedOperation;
    }
    auto error = lfs_file_truncate(&lfs, &lfsFile_, newSize);
    if(error < 0)
    {
        return static_cast<ErrorCode>(error);
    }
    return outcome_v2::success();
}


auto File::SeekAbsolute(int offset) const -> Result<int>
{
    return Seek(offset, LFS_SEEK_SET);
}


auto File::SeekRelative(int offset) const -> Result<int>
{
    return Seek(offset, LFS_SEEK_CUR);
}


auto File::Size() const -> Result<std::size_t>
{
    if(not persistentVariables.Load<"flashIsWorking">())
    {
        return ErrorCode::io;
    }
    if(not isOpen_)
    {
        return ErrorCode::fileNotOpen;
    }
    auto size = lfs_file_size(&lfs, &lfsFile_);
    if(size >= 0)
    {
        return static_cast<std::size_t>(size);
    }
    return static_cast<ErrorCode>(size);
}


auto File::Close() const -> Result<void>
{
    if(not persistentVariables.Load<"flashIsWorking">())
    {
        return ErrorCode::io;
    }
    if(not isOpen_)
    {
        return outcome_v2::success();
    }
    auto error = lfs_remove(&lfs, lockFilePath_.c_str());
    if(error != 0)
    {
        // Close file even when removing the lock file failed, otherwise we could leak memory
        (void)CloseAndKeepLockFile();
        return static_cast<ErrorCode>(error);
    }
    return CloseAndKeepLockFile();
}


auto File::Flush() -> Result<void>
{
    if(not persistentVariables.Load<"flashIsWorking">())
    {
        return ErrorCode::io;
    }
    if(not isOpen_)
    {
        return ErrorCode::fileNotOpen;
    }
    if((openFlags_ & LFS_O_WRONLY) == 0U)
    {
        return ErrorCode::unsupportedOperation;
    }
    auto error = lfs_file_sync(&lfs, &lfsFile_);
    if(error == 0)
    {
        return outcome_v2::success();
    }
    return static_cast<ErrorCode>(error);
}


// This function handles the weird way in which lfs_file_t objects are moved. It is more efficient
// than using the usual copy-and-swap idiom, because the latter would require to open and close the
// file more often.
auto File::MoveConstructFrom(File * other) noexcept -> void
{
    path_ = other->path_;
    lockFilePath_ = other->lockFilePath_;
    openFlags_ = other->openFlags_;
    lfsFile_ = other->lfsFile_;
    isOpen_ = other->isOpen_;
    if(other->path_.empty())
    {
        return;
    }
    if(persistentVariables.Load<"flashIsWorking">())
    {
        auto error = lfs_file_opencfg(
            &lfs, &lfsFile_, path_.c_str(), static_cast<int>(openFlags_), &lfsFileConfig_);
        isOpen_ = error == 0;
    }
    else
    {
        path_ = "";
        lockFilePath_ = "";
        openFlags_ = 0;
        lfsFile_ = {};
        isOpen_ = false;
    }
    (void)other->CloseAndKeepLockFile();
    other->path_ = "";
    other->lockFilePath_ = "";
    other->openFlags_ = 0;
    other->lfsFile_ = {};
}


auto File::CreateLockFile() const noexcept -> Result<void>
{
    auto lfsLockFile = lfs_file_t{};
    auto lockFileBuffer = std::array<Byte, lfsCacheSize>{};
    auto lfsLockFileConfig = lfs_file_config{.buffer = lockFileBuffer.data()};
    auto error = lfs_file_opencfg(&lfs,
                                  &lfsLockFile,
                                  lockFilePath_.c_str(),
                                  static_cast<unsigned int>(LFS_O_RDWR) | LFS_O_CREAT | LFS_O_EXCL,
                                  &lfsLockFileConfig);
    if(error == 0)
    {
        error = lfs_file_close(&lfs, &lfsLockFile);
    }
    if(error == 0)
    {
        return outcome_v2::success();
    }
    return static_cast<ErrorCode>(error);
}


auto File::Read(void * buffer, std::size_t size) const -> Result<int>
{
    if(not persistentVariables.Load<"flashIsWorking">())
    {
        return ErrorCode::io;
    }
    if(not isOpen_)
    {
        return ErrorCode::fileNotOpen;
    }
    if((openFlags_ & LFS_O_RDONLY) == 0U)
    {
        return ErrorCode::unsupportedOperation;
    }
    auto nReadBytes = lfs_file_read(&lfs, &lfsFile_, buffer, static_cast<lfs_size_t>(size));
    if(nReadBytes >= 0)
    {
        return nReadBytes;
    }
    return static_cast<ErrorCode>(nReadBytes);
}


auto File::Write(void const * buffer, std::size_t size) -> Result<int>
{
    if(not persistentVariables.Load<"flashIsWorking">())
    {
        return ErrorCode::io;
    }
    if(not isOpen_)
    {
        return ErrorCode::fileNotOpen;
    }
    if((openFlags_ & LFS_O_WRONLY) == 0U)
    {
        return ErrorCode::unsupportedOperation;
    }
    auto nWrittenBytes = lfs_file_write(&lfs, &lfsFile_, buffer, static_cast<lfs_size_t>(size));
    if(nWrittenBytes >= 0)
    {
        return nWrittenBytes;
    }
    return static_cast<ErrorCode>(nWrittenBytes);
}


auto File::Seek(int offset, int whence) const -> Result<int>
{
    if(not persistentVariables.Load<"flashIsWorking">())
    {
        return ErrorCode::io;
    }
    if(not isOpen_)
    {
        return ErrorCode::fileNotOpen;
    }
    auto errorOrNewPosition = lfs_file_seek(&lfs, &lfsFile_, offset, whence);
    if(errorOrNewPosition < 0)
    {
        return static_cast<ErrorCode>(errorOrNewPosition);
    }
    return errorOrNewPosition;
}


auto File::CloseAndKeepLockFile() const -> Result<void>
{
    if(not isOpen_)
    {
        return outcome_v2::success();
    }
    // lfs_file_close frees buffers and needs to be called even when flashIsWorking == false
    auto error = lfs_file_close(&lfs, &lfsFile_);
    if(error != 0)
    {
        return static_cast<ErrorCode>(error);
    }
    isOpen_ = false;
    return outcome_v2::success();
}
}
