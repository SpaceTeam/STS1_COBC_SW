#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>


namespace sts1cobcsw
{
constexpr auto IsEduError(ErrorCode error) -> bool
{
    return ErrorCode::invalidAnswer <= error and error <= ErrorCode::eduIsNotAlive;
}


constexpr auto IsFileSystemError(ErrorCode errorCode) -> bool
{
    return errorCode == ErrorCode::fileNotOpen or errorCode == ErrorCode::unsupportedOperation
        or errorCode == ErrorCode::fileLocked or errorCode < ErrorCode(0);
}


constexpr auto ToCZString(ErrorCode errorCode) -> char const *
{
    switch(errorCode)
    {
        // Littlefs (negative values)
        case ErrorCode::io:
            return "io";
        case ErrorCode::corrupt:
            return "corrupt";
        case ErrorCode::notFound:
            return "notFound";
        case ErrorCode::alreadyExists:
            return "alreadyExists";
        case ErrorCode::notADirectory:
            return "notADirectory";
        case ErrorCode::isADirectory:
            return "isADirectory";
        case ErrorCode::notEmpty:
            return "notEmpty";
        case ErrorCode::badFileNumber:
            return "badFileNumber";
        case ErrorCode::fileTooLarge:
            return "fileTooLarge";
        case ErrorCode::invalidParameter:
            return "invalidParameter";
        case ErrorCode::noSpace:
            return "noSpace";
        case ErrorCode::noMemory:
            return "noMemory";
        case ErrorCode::noAttribute:
            return "noAttribute";
        case ErrorCode::nameTooLong:
            return "nameTooLong";
        // General (from here on positive values)
        case ErrorCode::timeout:
            return "timeout";
        // File system
        case ErrorCode::fileNotOpen:
            return "fileNotOpen";
        case ErrorCode::unsupportedOperation:
            return "unsupportedOperation";
        case ErrorCode::fileLocked:
            return "fileLocked";
        // EDU
        case ErrorCode::invalidAnswer:
            return "invalidAnswer";
        case ErrorCode::nack:
            return "nack";
        case ErrorCode::tooManyNacks:
            return "tooManyNacks";
        case ErrorCode::wrongChecksum:
            return "wrongChecksum";
        case ErrorCode::dataPacketTooLong:
            return "dataPacketTooLong";
        case ErrorCode::invalidStatusType:
            return "invalidStatusType";
        case ErrorCode::invalidLength:
            return "invalidLength";
        case ErrorCode::tooManyDataPackets:
            return "tooManyDataPackets";
        case ErrorCode::eduIsNotAlive:
            return "eduIsNotAlive";
        case ErrorCode::invalidEduProgramFilename:
            return "invalidEduProgramFilename";
        // Mailbox
        case ErrorCode::full:
            return "full";
        case ErrorCode::empty:
            return "empty";
        // RF protocols
        case ErrorCode::errorCorrectionFailed:
            return "errorCorrectionFailed";
        case ErrorCode::dataFieldTooShort:
            return "dataFieldTooShort";
        case ErrorCode::invalidTransferFrame:
            return "invalidTransferFrame";
        case ErrorCode::invalidSpacecraftId:
            return "invalidSpacecraftId";
        case ErrorCode::invalidVcid:
            return "invalidVcid";
        case ErrorCode::invalidFrameLength:
            return "invalidFrameLength";
        case ErrorCode::invalidSecurityParameterIndex:
            return "invalidSecurityParameterIndex";
        case ErrorCode::authenticationFailed:
            return "authenticationFailed";
        case ErrorCode::bufferTooSmall:
            return "bufferTooSmall";
        case ErrorCode::invalidSpacePacket:
            return "invalidSpacePacket";
        case ErrorCode::invalidApid:
            return "invalidApid";
        case ErrorCode::invalidPacketDataLength:
            return "invalidPacketDataLength";
        case ErrorCode::emptyPayload:
            return "emptyPayload";
        case ErrorCode::invalidMessageTypeId:
            return "invalidMessageTypeId";
        case ErrorCode::invalidSourceId:
            return "invalidSourceId";
        case ErrorCode::invalidApplicationData:
            return "invalidApplicationData";
        case ErrorCode::invalidDataLength:
            return "invalidDataLength";
        case ErrorCode::invalidDataArea:
            return "invalidDataArea";
        case ErrorCode::invalidParameterId:
            return "invalidParameterId";
        case ErrorCode::emptyFilePath:
            return "emptyFilePath";
        case ErrorCode::invalidPartitionId:
            return "invalidPartitionId";
        case ErrorCode::invalidProtocolDataUnit:
            return "invalidProtocolDataUnit";
        case ErrorCode::invalidPduDataLength:
            return "invalidPduDataLength";
        case ErrorCode::invalidEntityId:
            return "invalidEntityId";
        case ErrorCode::invalidFileDirectiveCode:
            return "invalidFileDirectiveCode";
        case ErrorCode::invalidFaultLocation:
            return "invalidFaultLocation";
        case ErrorCode::invalidAckPduDirectiveCode:
            return "invalidAckPduDirectiveCode";
        case ErrorCode::invalidDirectiveSubtypeCode:
            return "invalidDirectiveSubtypeCode";
        case ErrorCode::invalidNakPdu:
            return "invalidNakPdu";
        // File transfer
        case ErrorCode::entityIdsAreIdentical:
            return "entityIdsAreIdentical";
        case ErrorCode::invalidCubeSatFilePath:
            return "invalidCubeSatFilePath";
        case ErrorCode::invalidFirmwarePath:
            return "invalidFirmwarePath";
        case ErrorCode::fileTransferInterrupted:
            return "fileTransferInterrupted";
        case ErrorCode::fileTransferCanceled:
            return "fileTransferCanceled";
        case ErrorCode::positiveAckLimitReached:
            return "positiveAckLimitReached";
        case ErrorCode::fileSizeError:
            return "fileSizeError";
        case ErrorCode::nakLimitReached:
            return "nakLimitReached";
        case ErrorCode::inactivityDetected:
            return "inactivityDetected";
        case ErrorCode::wrongPduType:
            return "wrongPduType";
        case ErrorCode::receivedNoFileData:
            return "receivedNoFileData";
        // Firmware
        case ErrorCode::misaligned:
            return "misaligned";
        case ErrorCode::eraseFailed:
            return "eraseFailed";
        case ErrorCode::programFailed:
            return "programFailed";
    }
    return "unknown error code";
}
}
