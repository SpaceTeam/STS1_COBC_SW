#pragma once


// TODO: Rename this file to EduEnums.hpp or merge it with EduStructs to EduTypes.
namespace sts1cobcsw::periphery
{
// TODO: Remove error prefix in enum values, the type already has "error" in it
enum class EduErrorCode
{
    noErrorCodeSet,
    success,
    successEof,
    invalidResult,
    bufferTooSmall,
    uartNotInitialized,
    timeout,
    nack,
    // Separate errors for the SendData function
    // to differentiate where the error occurred
    invalidDataResult,
    sendDataTooLong,
    receiveDataTooLong,
    tooManyNacks,
    dataTimeout,
    wrongChecksum,
    invalidStatusType,
    invalidLength,
    invalidCommand,
    noResultAvailable
};

enum class EduStatusType
{
    noEvent,
    programFinished,
    resultsReady,
    invalid,
};

}
