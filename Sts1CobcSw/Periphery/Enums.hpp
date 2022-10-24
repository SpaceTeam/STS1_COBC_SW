#pragma once


// TODO: Rename this file to EduEnums.hpp or merge it with EduStructs to EduTypes.
namespace sts1cobcsw::periphery
{
// TODO: Remove error prefix in enum values, the type already has "error" in it
enum class EduErrorCode
{
    success,
    successEof,
    invalidResult,
    bufferTooSmall,
    uartNotInitialized,
    timeout,
    nack,
    // Separate errors for the SendData function
    // to differentiate where the error occured
    invalidDataResult,
    sendDataTooLong,
    receiveDataTooLong,
    tooManyNacks,
    dataTimeout,
    wrongChecksum
};

enum class EduStatusType
{
    invalid,
    noEvent,
    programFinished,
    resultsReady,
};

}