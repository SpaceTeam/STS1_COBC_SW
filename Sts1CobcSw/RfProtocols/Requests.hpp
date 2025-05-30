#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <span>

#include "Sts1CobcSw/RfProtocols/TcSpacePacketSecondaryHeader.hpp"


namespace sts1cobcsw
{
struct Request
{
    using SecondaryHeader = tc::SpacePacketSecondaryHeader;

    SecondaryHeader secondaryHeader;
    std::span<Byte const> dataField;
};


[[nodiscard]] auto ParseAsRequest(std::span<Byte const> buffer) -> Result<Request>;
}
