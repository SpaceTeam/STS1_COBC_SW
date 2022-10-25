#pragma once

namespace sts1cobcsw::periphery
{
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
    errorRecvDataTooLong,
    errorNackRetries,
    errorDataTimeout,
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

