#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/TcSpacePacketSecondaryHeader.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <span>


namespace sts1cobcsw
{
struct Request
{
    tc::SpacePacketSecondaryHeader packetSecondaryHeader;
    std::span<Byte const> applicationData;
};


[[nodiscard]] auto ParseAsRequest(std::span<Byte const> buffer) -> Result<Request>;
}
