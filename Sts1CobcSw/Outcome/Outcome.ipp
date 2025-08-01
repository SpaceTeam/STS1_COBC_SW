#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>


namespace sts1cobcsw
{
constexpr auto ToCZString(ErrorCode errorCode) -> char const *
{
    switch(errorCode)
    {
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
        case ErrorCode::tooLarge:
            return "tooLarge";
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
        case ErrorCode::timeout:
            return "timeout";
        case ErrorCode::fileNotOpen:
            return "fileNotOpen";
        case ErrorCode::unsupportedOperation:
            return "unsupportedOperation";
        case ErrorCode::fileLocked:
            return "fileLocked";
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
        case ErrorCode::full:
            return "full";
        case ErrorCode::empty:
            return "empty";
        case ErrorCode::errorCorrectionFailed:
            return "errorCorrectionFailed";
        case ErrorCode::bufferTooSmall:
            return "bufferTooSmall";
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
