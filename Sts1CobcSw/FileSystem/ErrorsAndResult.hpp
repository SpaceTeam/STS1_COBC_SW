#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>

#include <littlefs/lfs.h>


namespace sts1cobcsw::fs
{
enum class ErrorCode
{
    io = LFS_ERR_IO,                    // Error during device operation
    corrupt = LFS_ERR_CORRUPT,          // Corrupted
    noDirectoryEntry = LFS_ERR_NOENT,   // No directory entry
    alreadyExists = LFS_ERR_EXIST,      // Entry already exists
    notADirectory = LFS_ERR_NOTDIR,     // Entry is not a dir
    isADirectory = LFS_ERR_ISDIR,       // Entry is a dir
    notEmpty = LFS_ERR_NOTEMPTY,        // Dir is not empty
    badFileNumber = LFS_ERR_BADF,       // Bad file number
    tooLarge = LFS_ERR_FBIG,            // File too large
    invalidParameter = LFS_ERR_INVAL,   // Invalid parameter
    noSpace = LFS_ERR_NOSPC,            // No space left on device
    noMemory = LFS_ERR_NOMEM,           // No more memory available
    noAttribute = LFS_ERR_NOATTR,       // No data/attr available
    nameTooLong = LFS_ERR_NAMETOOLONG,  // File name too long
    fileNotOpen = 1,
    unsupportedOperation,
};


template<typename T>
using Result = outcome_v2::experimental::status_result<T, ErrorCode, RebootPolicy>;
}
