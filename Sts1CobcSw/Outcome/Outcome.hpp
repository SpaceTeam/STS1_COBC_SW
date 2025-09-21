#pragma once


#include <littlefs/lfs.h>

#ifndef BUILD_BOOTLOADER
    #include <rodos_no_using_namespace.h>

    #if defined(SYSTEM_ERROR2_NOT_POSIX)
        // NOLINTNEXTLINE(*macro-usage)
        #define SYSTEM_ERROR2_FATAL(msg) RODOS::hwResetAndReboot()
    #endif

    #include <outcome-experimental.hpp>  // IWYU pragma: export
#endif

#include <cstdint>


namespace sts1cobcsw
{
enum class ErrorCode : std::int8_t  // NOLINT
{
    // Littlefs (negative values)
    io = LFS_ERR_IO,                    // Error during device operation
    corrupt = LFS_ERR_CORRUPT,          // Corrupted
    notFound = LFS_ERR_NOENT,           // No directory entry (I think this means "entry not found")
    alreadyExists = LFS_ERR_EXIST,      // Entry already exists
    notADirectory = LFS_ERR_NOTDIR,     // Entry is not a dir
    isADirectory = LFS_ERR_ISDIR,       // Entry is a dir
    notEmpty = LFS_ERR_NOTEMPTY,        // Dir is not empty
    badFileNumber = LFS_ERR_BADF,       // Bad file number
    fileTooLarge = LFS_ERR_FBIG,        // File too large
    invalidParameter = LFS_ERR_INVAL,   // Invalid parameter
    noSpace = LFS_ERR_NOSPC,            // No space left on device
    noMemory = LFS_ERR_NOMEM,           // No more memory available
    noAttribute = LFS_ERR_NOATTR,       // No data/attr available
    nameTooLong = LFS_ERR_NAMETOOLONG,  // File name too long
    // General (from here on positive values)
    timeout = 1,
    // File system
    fileNotOpen,
    unsupportedOperation,
    fileLocked,
    // EDU
    invalidAnswer,
    nack,
    tooManyNacks,
    wrongChecksum,
    dataPacketTooLong,
    invalidStatusType,
    invalidLength,
    tooManyDataPackets,
    eduIsNotAlive,
    // Mailbox
    full,
    empty,
    // RF protocols
    // TODO: Rework the EDU and RF protocol errors once they are fully implemented. Since we send
    // down the error code with the NACK report, we benefit from fine-grained error codes.
    errorCorrectionFailed,
    dataFieldTooShort,
    invalidTransferFrame,
    invalidSpacecraftId,
    invalidVcid,
    invalidFrameLength,
    invalidSecurityParameterIndex,
    authenticationFailed,
    bufferTooSmall,
    invalidSpacePacket,
    invalidApid,
    invalidPacketDataLength,
    emptyPayload,
    invalidMessageTypeId,
    invalidSourceId,
    invalidApplicationData,
    // TODO: Rename to invalidApplicationDataLength
    invalidDataLength,
    invalidDataArea,
    invalidParameterId,
    emptyFilePath,
    invalidPartitionId,
    invalidProtocolDataUnit,
    invalidPduDataLength,
    invalidEntityId,
    invalidFileDirectiveCode,
    invalidFaultLocation,
    invalidAckPduDirectiveCode,
    invalidDirectiveSubtypeCode,
    invalidNakPdu,
    // Firmware
    misaligned,
    eraseFailed,
    programFailed,
};


#ifndef BUILD_BOOTLOADER
// TODO: Think about printing more information. For example, we could print the error code in case
// the value check fails.
struct RebootPolicy : outcome_v2::experimental::policy::base
{
    template<class Impl>
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr void wide_value_check(Impl && self)
    {
        //! Call RODOS::hwResetAndReboot() whenever .value() is called on an object that does not
        //! contain a value
        if(!base::_has_value(std::forward<Impl>(self)))
        {
            RODOS::PRINTF(
                "Error: The value is not present. Performing hardware reset and reboot.\n");
    #ifdef __linux__
            RODOS::isShuttingDown = true;
            // NOLINTNEXTLINE(concurrency-mt-unsafe)
            std::exit(-1);
    #else
            RODOS::hwResetAndReboot();
    #endif
        }
    }

    template<class Impl>
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr void wide_error_check(Impl && self)
    {
        //! Call RODOS::hwResetAndReboot() whenever .error() is called on an object that does not
        //! contain an error
        if(!base::_has_error(std::forward<Impl>(self)))
        {
            RODOS::PRINTF(
                "Error: The error is not present. Performing hardware reset and reboot.\n");
    #ifdef __linux__
            RODOS::isShuttingDown = true;
            // NOLINTNEXTLINE(concurrency-mt-unsafe)
            std::exit(-1);
    #else
            RODOS::hwResetAndReboot();
    #endif
        }
    }

    template<class Impl>
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr void wide_exception_check(Impl && self)
    {
        if(!base::_has_exception(std::forward<Impl>(self)))
        {
            RODOS::PRINTF(
                "Error: The exception is not present. Performing hardware reset and reboot.\n");
    #ifdef __linux__
            RODOS::isShuttingDown = true;
            // NOLINTNEXTLINE(concurrency-mt-unsafe)
            std::exit(-1);
    #else
            RODOS::hwResetAndReboot();
    #endif
        }
    }
};


template<typename T>
using Result = outcome_v2::experimental::status_result<T, ErrorCode, RebootPolicy>;
#endif


constexpr auto IsEduError(ErrorCode error) -> bool;
constexpr auto ToCZString(ErrorCode errorCode) -> char const *;
}


#include <Sts1CobcSw/Outcome/Outcome.ipp>  // IWYU pragma: keep
