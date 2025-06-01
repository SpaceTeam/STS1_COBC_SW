#include <Sts1CobcSw/FileSystem/DirectoryIterator.hpp>

#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>

#include <littlefs/lfs.h>


namespace sts1cobcsw::fs
{
namespace
{
auto & lfs = internal::lfs;
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
    if(persistentVariables.template Load<"flashIsWorking">() and not path_.empty())
    {
        (void)lfs_dir_close(&lfs, &lfsDirectory_);
    }
}


auto MakeIterator(Path const & path) -> Result<DirectoryIterator>
{
    if(not persistentVariables.template Load<"flashIsWorking">())
    {
        return ErrorCode::io;
    }
    auto iterator = DirectoryIterator{};
    auto error = lfs_dir_open(&lfs, &iterator.lfsDirectory_, path.c_str());
    if(error == 0)
    {
        iterator.path_ = path;
        iterator.ReadNextDirectoryEntry();
        return iterator;
    }
    return static_cast<ErrorCode>(error);
}


// We do not compare the lfsErrorCode_ here, because we want all iterators that differ just by
// the error code to be considered equal to the end iterator. This stops range-based for loops
// and algorithms early when an error occurs. See also ReadNextDirectoryEntry().
auto operator==(DirectoryIterator const & lhs, DirectoryIterator const & rhs) -> bool
{
    return lhs.path_ == rhs.path_ and lhs.lfsDirectory_.pos == rhs.lfsDirectory_.pos
        && lhs.directoryInfo_ == rhs.directoryInfo_;
}


auto DirectoryIterator::operator++() -> DirectoryIterator &
{
    if(not persistentVariables.template Load<"flashIsWorking">())
    {
        lfsErrorCode_ = LFS_ERR_IO;
        ResetEverythingExceptErrorCode();
    }
    else
    {
        ReadNextDirectoryEntry();
    }
    return *this;
}


auto DirectoryIterator::operator*() const -> Result<DirectoryInfo>
{
    if(lfsErrorCode_ > 0)
    {
        return directoryInfo_;
    }
    if(lfsErrorCode_ == 0)
    {
        // Dereferencing an end iterator is not supported
        return ErrorCode::unsupportedOperation;
    }
    return static_cast<ErrorCode>(lfsErrorCode_);
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
    if(persistentVariables.template Load<"flashIsWorking">())
    {
        if(not path_.empty())
        {
            (void)lfs_dir_close(&lfs, &lfsDirectory_);
        }
        path_ = other->path_;
        lfsDirectory_ = other->lfsDirectory_;
        directoryInfo_ = other->directoryInfo_;
        lfsErrorCode_ = other->lfsErrorCode_;
        if(path_.empty())
        {
            return;
        }
        lfsErrorCode_ = lfs_dir_open(&lfs, &lfsDirectory_, path_.c_str());
    }
    else
    {
        lfsErrorCode_ = LFS_ERR_IO;
    }
    if(lfsErrorCode_ != 0)
    {
        ResetEverythingExceptErrorCode();
        return;
    }
    while(lfsDirectory_.pos != other->lfsDirectory_.pos)
    {
        ReadNextDirectoryEntry();
        if(lfsErrorCode_ <= 0)
        {
            break;
        }
    }
}


auto DirectoryIterator::ReadNextDirectoryEntry() -> void
{
    if(path_.empty())
    {
        return;
    }
    auto lfsInfo = lfs_info{};
    lfsErrorCode_ = lfs_dir_read(&lfs, &lfsDirectory_, &lfsInfo);
    // If lfsErrorCode_ == 0 we are at the end of the directory, if lfsErrorCode_ < 0 an error
    // occurred. In both cases we can't iterate any further so we close the directory and go
    // into an end-iterator-like state, i.e., we reset everything except the error code. These
    // states all compare equal to the end iterator (see operator==()) thereby stopping
    // iteration.
    if(lfsErrorCode_ <= 0)
    {
        (void)lfs_dir_close(&lfs, &lfsDirectory_);
        ResetEverythingExceptErrorCode();
        return;
    }
    directoryInfo_.name = etl::make_string(lfsInfo.name);
    directoryInfo_.size = lfsInfo.size;
    directoryInfo_.type = lfsInfo.type == LFS_TYPE_REG ? EntryType::file : EntryType::directory;
}


auto DirectoryIterator::ResetEverythingExceptErrorCode() -> void
{
    path_ = "";
    lfsDirectory_ = {};
    directoryInfo_ = {};
}
}
