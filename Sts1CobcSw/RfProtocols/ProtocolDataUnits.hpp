#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/ProtocolDataUnitHeader.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <etl/utility.h>
#include <etl/vector.h>

#include <span>


namespace sts1cobcsw
{
namespace tc
{
struct ProtocolDataUnit
{
    using Header = ProtocolDataUnitHeader;

    Header header;
    etl::vector<Byte, tc::maxPduDataLength> dataField;
};
}


[[nodiscard]] auto ParseAsProtocolDataUnit(std::span<Byte const> buffer)
    -> Result<tc::ProtocolDataUnit>;
}
