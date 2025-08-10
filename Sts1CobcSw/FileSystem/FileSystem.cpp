#include <Sts1CobcSw/FileSystem/FileSystem.hpp>

#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>

#include <etl/string_view.h>

#include <cassert>


namespace sts1cobcsw::fs
{
namespace internal
{
lfs_t lfs{};
}

namespace
{
auto & lfs = internal::lfs;
}


// FIXME: For some reason this allocates 1024 bytes on the heap. With LFS_NO_MALLOC defined, it
// crashes with a SEGFAULT.
auto Mount() -> Result<void>
{
    if(not persistentVariables.Load<"flashIsWorking">())
    {
        return ErrorCode::io;
    }
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
    // Allow unmount when flash is not working since it only frees memory
    auto error = lfs_unmount(&lfs);
    if(error != 0)
    {
        return static_cast<ErrorCode>(error);
    }
    return outcome_v2::success();
}


auto CreateDirectory(Path const & path) -> Result<void>
{
    assert(path.back() != '/');
    if(not persistentVariables.Load<"flashIsWorking">())
    {
        return ErrorCode::io;
    }
    auto error = lfs_mkdir(&lfs, path.c_str());
    if(error == 0)
    {
        return outcome_v2::success();
    }
    return static_cast<ErrorCode>(error);
}


auto Remove(Path const & path) -> Result<void>
{
    if(not persistentVariables.Load<"flashIsWorking">())
    {
        return ErrorCode::io;
    }
    auto info = lfs_info{};
    auto error = lfs_stat(&lfs, BuildLockFilePath(path).c_str(), &info);
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
    if(not persistentVariables.Load<"flashIsWorking">())
    {
        return ErrorCode::io;
    }
    auto error = lfs_remove(&lfs, BuildLockFilePath(path).c_str());
    if(error == 0 || error == LFS_ERR_NOENT)
    {
        return Remove(path);
    }
    return static_cast<ErrorCode>(error);
}


auto BuildLockFilePath(Path path) -> Path
{
    return path.append(".lock");
}


auto FileSize(Path const & path) -> Result<std::uint32_t>
{
    if(not persistentVariables.Load<"flashIsWorking">())
    {
        return ErrorCode::io;
    }
    auto info = lfs_info{};
    auto error = lfs_stat(&lfs, path.c_str(), &info);
    if(error == 0)
    {
        if(info.type == LFS_TYPE_REG)
        {
            return info.size;
        }
        return ErrorCode::isADirectory;
    }
    return static_cast<ErrorCode>(error);
}


auto IsLocked(Path const & path) -> Result<bool>
{
    if(not persistentVariables.Load<"flashIsWorking">())
    {
        return ErrorCode::io;
    }
    auto info = lfs_info{};
    // Get stats of lock file to see if it exists
    auto error = lfs_stat(&lfs, BuildLockFilePath(path).c_str(), &info);
    if(error == 0)
    {
        return true;
    }
    if(error == LFS_ERR_NOENT)
    {
        return false;
    }
    return static_cast<ErrorCode>(error);
}


auto IsLockFile(Path const & path) -> bool
{
    return etl::string_view(path).ends_with(".lock");
}
}
