#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/MessageTypeIdFields.hpp>
#include <Sts1CobcSw/RfProtocols/Requests.hpp>
#include <Sts1CobcSw/RfProtocols/TcSpacePacketSecondaryHeader.hpp>

#include <type_traits>

namespace sts1cobcsw
{

[[nodiscard]] auto ParseAsRequest(std::span<Byte const> buffer) -> Result<Request>
{
    if(buffer.size() < tc::packetSecondaryHeaderLength)
    {
        return ErrorCode::bufferTooSmall;
    }
    auto request = Request{.packetSecondaryHeader = tc::SpacePacketSecondaryHeader(),
                           .applicationData = buffer.subspan(tc::packetSecondaryHeaderLength)};
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(),
                                                       &request.packetSecondaryHeader);
    auto packetIsValid =
        request.packetSecondaryHeader.tcPacketPusVersionNumber == tc::packetPusVersionNumber;
    if(not packetIsValid)
    {
        return ErrorCode::invalidSpacePacket;
    }
    if(not IsValid(request.packetSecondaryHeader.messageTypeId))
    {
        return ErrorCode::invalidMessageTypeId;
    }
    if(not IsValid(request.packetSecondaryHeader.sourceId))
    {
        return ErrorCode::invalidSourceId;
    }
    return request;
}

}
