#include <Sts1CobcSw/FileSystem/FileSystem.hpp>

#include <Sts1CobcSw/FramSections/FramLayout.hpp>

#include <etl/to_string.h>


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
    if(not persistentVariables.template Load<"flashIsWorking">())
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
    if(not persistentVariables.template Load<"flashIsWorking">())
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
    if(not persistentVariables.template Load<"flashIsWorking">())
    {
        return ErrorCode::io;
    }
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
    if(not persistentVariables.template Load<"flashIsWorking">())
    {
        return ErrorCode::io;
    }
    auto error = lfs_remove(&lfs, Path(path).append(".lock").c_str());
    if(error == 0 || error == LFS_ERR_NOENT)
    {
        return Remove(path);
    }
    return static_cast<ErrorCode>(error);
}
}
