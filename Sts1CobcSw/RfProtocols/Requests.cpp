#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/MessageTypeIdFields.hpp>
#include <Sts1CobcSw/RfProtocols/Requests.hpp>
#include <Sts1CobcSw/RfProtocols/TcSpacePacketSecondaryHeader.hpp>

#include <type_traits>

#include "Sts1CobcSw/RfProtocols/Configuration.hpp"
#include "Sts1CobcSw/RfProtocols/Id.hpp"

namespace sts1cobcsw
{

[[nodiscard]] auto ParseAsRequest(std::span<Byte const> buffer) -> Result<Request>
{
    if(buffer.size() < tc::packetSecondaryHeaderLength)
    {
        return ErrorCode::bufferTooSmall;
    }
    auto secondaryHeader = tc::SpacePacketSecondaryHeader();
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(), &secondaryHeader);
    auto packetIsValid = secondaryHeader.tcPacketPusVersionNumber == tc::packetPusVersionNumber;
    if(not packetIsValid)
    {
        return ErrorCode::invalidSpacePacket;
    }
    if(not IsValid(secondaryHeader.messageTypeId))
    {
        return ErrorCode::invalidMessageTypeId;
    }
    if(not IsValid(secondaryHeader.sourceId))
    {
        return ErrorCode::invalidSourceId;
    }

    return Request{.secondaryHeader = secondaryHeader,
                   .dataField = buffer.subspan(tc::packetSecondaryHeaderLength)};
}

}
