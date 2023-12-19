#pragma once


// TODO: Rename this file to Enums.hpp or merge it with EduStructs to EduTypes.
namespace sts1cobcsw::edu
{
enum class ErrorCode
{
    invalidResult = 1,
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


enum class StatusType
{
    noEvent,
    programFinished,
    resultsReady,
    invalid,
};
}
