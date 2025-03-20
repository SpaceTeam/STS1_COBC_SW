#include <Sts1CobcSw/FileSystem/LfsWrapper.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>

#include <littlefs/lfs.h>

#include <etl/string.h>
#include <etl/to_string.h>


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

auto CreateDirectory(Path const & path) -> Result<void>
{
    auto error = lfs_mkdir(&lfs, path.c_str());
    if(error == 0)
    {
        return outcome_v2::success();
    }
    return static_cast<ErrorCode>(error);
}


auto Remove(Path const & path) -> Result<void>
{
    auto info = lfs_info{};
    auto error = lfs_stat(&lfs, Path(path).append(".lock").c_str(), &info);
    auto lockFileExists = error == 0;
    if(lockFileExists)
    {
        return ErrorCode::fileLocked;
    }
    error = lfs_remove(&lfs, path.c_str());
    if(error == 0)
    {
        return outcome_v2::success();
    }
    return static_cast<ErrorCode>(error);
}


auto ForceRemove(Path const & path) -> Result<void>
{
    auto error = lfs_remove(&lfs, Path(path).append(".lock").c_str());
    if(error == 0 || error == LFS_ERR_NOENT)
    {
        return Remove(path);
    }
    return static_cast<ErrorCode>(error);
}


auto MakeIterator(Path const & path) -> Result<DirectoryIterator>
{
    auto iterator = DirectoryIterator();
    auto error = lfs_dir_open(&lfs, &iterator.lfsDirectory_, path.c_str());
    if(error == 0)
    {
        iterator.path_ = path;
        iterator.isOpen_ = true;
        iterator.ReadNextDirectoryEntry();
        return iterator;
    }
    return static_cast<ErrorCode>(error);
}


DirectoryIterator::DirectoryIterator(DirectoryIterator const & other) noexcept
{
    CopyConstructFrom(&other);
}


DirectoryIterator::DirectoryIterator(DirectoryIterator && other) noexcept
{
    CopyConstructFrom(&other);
}


auto DirectoryIterator::operator=(DirectoryIterator const & other) noexcept -> DirectoryIterator &
{
    if(this != &other)
    {
        CopyConstructFrom(&other);
    }
    return *this;
}


auto DirectoryIterator::operator=(DirectoryIterator && other) noexcept -> DirectoryIterator &
{
    if(this != &other)
    {
        CopyConstructFrom(&other);
    }
    return *this;
}


DirectoryIterator::~DirectoryIterator()
{
    if(isOpen_)
    {
        (void)lfs_dir_close(&lfs, &lfsDirectory_);
    }
}


auto DirectoryIterator::operator++() -> DirectoryIterator &
{
    ReadNextDirectoryEntry();
    return *this;
}


auto DirectoryIterator::operator*() const -> Result<DirectoryInfo>
{
    if(not isOpen_)
    {
        // Dereferencing an iterator after end or when an error occurred is undefined behavior
        return ErrorCode::noDirectoryEntry;
    }
    if(lfsFileErrorCode_ > 0)
    {
        return directoryInfo_;
    }
    return static_cast<ErrorCode>(lfsFileErrorCode_);
}


auto DirectoryIterator::operator==(DirectoryIterator const & other) const -> bool
{
    // If one still has a open lfs_dir and the other not -> end not reached -> not equal
    if((isOpen_ && not other.isOpen_) || (not isOpen_ && other.isOpen_))
    {
        return false;
    }
    if(not isOpen_ && not other.isOpen_)
    {
        return true;
    }
    if(path_ != other.path_)
    {
        return false;
    }
    return lfsDirectory_.pos == other.lfsDirectory_.pos;
}


auto DirectoryIterator::operator!=(DirectoryIterator const & other) const -> bool
{
    return not(*this == other);
}


auto DirectoryIterator::begin() const -> DirectoryIterator
{
    return *this;
}


auto DirectoryIterator::end() -> DirectoryIterator
{
    return DirectoryIterator{};
}


auto DirectoryIterator::CopyConstructFrom(DirectoryIterator const * other) noexcept -> void
{
    path_ = other->path_;
    if(not other->isOpen_)
    {
        isOpen_ = false;
        return;
    }
    auto error = lfs_dir_open(&lfs, &lfsDirectory_, path_.c_str());
    if(error != 0)
    {
        isOpen_ = false;
        return;
    }
    isOpen_ = true;
    while(lfsDirectory_.pos != other->lfsDirectory_.pos)
    {
        ReadNextDirectoryEntry();
        if(lfsFileErrorCode_ < 0)
        {
            isOpen_ = false;
            break;
        }
    }
}


auto DirectoryIterator::ReadNextDirectoryEntry() -> void
{
    if(not isOpen_)
    {
        return;
    }
    lfs_info lfsInfo{};
    lfsFileErrorCode_ = lfs_dir_read(&lfs, &lfsDirectory_, &lfsInfo);
    directoryInfo_.name = etl::make_string(lfsInfo.name);
    directoryInfo_.size = lfsInfo.size;
    directoryInfo_.type = static_cast<DirectoryInfo::EntryType>(lfsInfo.type);

    if(lfsFileErrorCode_ == 0)
    {
        isOpen_ = false;
        (void)lfs_dir_close(&lfs, &lfsDirectory_);
    }
}


auto Open(Path const & path, unsigned int flags) -> Result<File>
{
    auto file = File();
    // We need to create a temporary with Path(path) here since append() modifies the object and we
    // don't want to change the original path
    file.lockFilePath_ = Path(path).append(".lock");
    auto createLockFileResult = file.CreateLockFile();
    if(createLockFileResult.has_error())
    {
        return ErrorCode::fileLocked;
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


auto File::Close() const -> Result<void>
{
    if(not isOpen_)
    {
        return outcome_v2::success();
    }
    auto error = lfs_remove(&lfs, lockFilePath_.c_str());
    if(error != 0)
    {
        return static_cast<ErrorCode>(error);
    }
    return CloseAndKeepLockFile();
}


auto File::Flush() -> Result<void>
{
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
        lockFilePath_ = other->lockFilePath_;
        openFlags_ = other->openFlags_;
        isOpen_ = true;
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


[[nodiscard]] auto File::Seek(int offset, unsigned int flag) -> Result<int>
{
    if(not isOpen_)
    {
        return ErrorCode::fileNotOpen;
    }
    auto error = lfs_file_seek(&lfs, &lfsFile_, offset, static_cast<int>(flag));
    if(error != 0)
    {
        return static_cast<ErrorCode>(error);
    }
    return error;
}


auto File::CloseAndKeepLockFile() const -> Result<void>
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
