#include <Tests/CatchRodos/Stringification.hpp>
#include <Tests/Utility/Stringification.hpp>

#include <strong_type/type.hpp>


namespace sts1cobcsw
{
auto Append(ValueString * string, ErrorCode const & errorCode) -> void
{
    switch(errorCode)
    {
        case ErrorCode::io:
            Append(string, "ErrorCode::io");
            break;
        case ErrorCode::corrupt:
            Append(string, "ErrorCode::corrupt");
            break;
        case ErrorCode::notFound:
            Append(string, "ErrorCode::notFound");
            break;
        case ErrorCode::alreadyExists:
            Append(string, "ErrorCode::alreadyExists");
            break;
        case ErrorCode::notADirectory:
            Append(string, "ErrorCode::notADirectory");
            break;
        case ErrorCode::isADirectory:
            Append(string, "ErrorCode::isADirectory");
            break;
        case ErrorCode::notEmpty:
            Append(string, "ErrorCode::notEmpty");
            break;
        case ErrorCode::badFileNumber:
            Append(string, "ErrorCode::badFileNumber");
            break;
        case ErrorCode::tooLarge:
            Append(string, "ErrorCode::tooLarge");
            break;
        case ErrorCode::invalidParameter:
            Append(string, "ErrorCode::invalidParameter");
            break;
        case ErrorCode::noSpace:
            Append(string, "ErrorCode::noSpace");
            break;
        case ErrorCode::noMemory:
            Append(string, "ErrorCode::noMemory");
            break;
        case ErrorCode::noAttribute:
            Append(string, "ErrorCode::noAttribute");
            break;
        case ErrorCode::nameTooLong:
            Append(string, "ErrorCode::nameTooLong");
            break;
        case ErrorCode::timeout:
            Append(string, "ErrorCode::timeout");
            break;
        case ErrorCode::fileNotOpen:
            Append(string, "ErrorCode::fileNotOpen");
            break;
        case ErrorCode::unsupportedOperation:
            Append(string, "ErrorCode::unsupportedOperation");
            break;
        case ErrorCode::fileLocked:
            Append(string, "ErrorCode::fileLocked");
            break;
        case ErrorCode::invalidAnswer:
            Append(string, "ErrorCode::invalidAnswer");
            break;
        case ErrorCode::nack:
            Append(string, "ErrorCode::nack");
            break;
        case ErrorCode::tooManyNacks:
            Append(string, "ErrorCode::tooManyNacks");
            break;
        case ErrorCode::wrongChecksum:
            Append(string, "ErrorCode::wrongChecksum");
            break;
        case ErrorCode::dataPacketTooLong:
            Append(string, "ErrorCode::dataPacketTooLong");
            break;
        case ErrorCode::invalidStatusType:
            Append(string, "ErrorCode::invalidStatusType");
            break;
        case ErrorCode::invalidLength:
            Append(string, "ErrorCode::invalidLength");
            break;
        case ErrorCode::tooManyDataPackets:
            Append(string, "ErrorCode::tooManyDataPackets");
            break;
        case ErrorCode::full:
            Append(string, "ErrorCode::full");
            break;
        case ErrorCode::empty:
            Append(string, "ErrorCode::empty");
            break;
        case ErrorCode::bufferTooSmall:
            Append(string, "ErrorCode::bufferTooSmall");
            break;
        case ErrorCode::invalidTransferFrame:
            Append(string, "ErrorCode::invalidTransferFrame");
            break;
        case ErrorCode::invalidSpacecraftId:
            Append(string, "ErrorCode::invalidSpacecraftId");
            break;
        case ErrorCode::invalidVcid:
            Append(string, "ErrorCode::invalidVcid");
            break;
        case ErrorCode::invalidFrameLength:
            Append(string, "ErrorCode::invalidFrameLength");
            break;
        case ErrorCode::invalidSecurityParameterIndex:
            Append(string, "ErrorCode::invalidSecurityParameterIndex");
            break;
        case ErrorCode::authenticationFailed:
            Append(string, "ErrorCode::authenticationFailed");
            break;
        case ErrorCode::invalidSpacePacket:
            Append(string, "ErrorCode::invalidSpacePacket");
            break;
        case ErrorCode::invalidApid:
            Append(string, "ErrorCode::invalidApid");
            break;
        case ErrorCode::invalidPacketLength:
            Append(string, "ErrorCode::invalidPacketLength");
            break;
        case ErrorCode::emptyPayload:
            Append(string, "ErrorCode::emptyPayload");
            break;
        case ErrorCode::invalidMessageTypeId:
            Append(string, "ErrorCode::invalidMessageTypeId");
            break;
        case ErrorCode::invalidSourceId:
            Append(string, "ErrorCode::invalidSourceId");
            break;
        case ErrorCode::invalidApplicationData:
            Append(string, "ErrorCode::invalidApplicationData");
            break;
        case ErrorCode::invalidDataLength:
            Append(string, "ErrorCode::invalidDataLength");
            break;
        case ErrorCode::emptyFilePath:
            Append(string, "ErrorCode::emptyFilePath");
            break;
        case ErrorCode::misaligned:
            Append(string, "ErrorCode::misaligned");
            break;
        case ErrorCode::eraseFailed:
            Append(string, "ErrorCode::eraseFailed");
            break;
        case ErrorCode::programFailed:
            Append(string, "ErrorCode::programFailed");
            break;
    }
}


auto Append(ValueString * string, RealTime const & realTime) -> void
{
    Append(string, value_of(realTime));
    Append(string, " s");
}
}
