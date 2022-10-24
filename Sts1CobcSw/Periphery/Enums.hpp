#pragma once


// TODO: Rename this file to EduEnums.hpp or merge it with EduStructs to EduTypes.
namespace sts1cobcsw::periphery
{
// TODO: Remove error prefix in enum values, the type already has "error" in it
enum class EduErrorCode
{
    success,
    successEof,
    errorBufferTooSmall,
    errorUartNotInitialized,
    errorTimeout,
    errorNack,
    errorInvalidResult,
    // Separate errors for the SendData function
    // to differentiate where the error occured
    errorSendDataTooLong,
    errorReceiveDataTooLong,
    errorNackRetries,
    errorDataTimeout,  // TODO: Why is errorTimeout not enough?
    errorDataInvalidResult,
    errorChecksum
};

enum class EduStatusType
{
    noEvent,
    programFinished,
    resultsReady,
    invalid
};

}