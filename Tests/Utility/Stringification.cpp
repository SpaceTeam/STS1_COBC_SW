#include <Tests/Utility/Stringification.hpp>


namespace sts1cobcsw
{
auto Append(ValueString * string, ErrorCode const & value) -> void
{
    switch(value)
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
    }
}
}
