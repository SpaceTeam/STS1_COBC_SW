#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/ProtocolDataUnitHeader.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <etl/utility.h>
#include <etl/vector.h>

#include <span>


namespace sts1cobcsw
{
namespace pdutype
{
inline constexpr auto fileDirective = UInt<1>(0);
inline constexpr auto fileData = UInt<1>(1);
}

namespace direction
{
inline constexpr auto towardsFileReceiver = UInt<1>(0);
inline constexpr auto towardsFileSender = UInt<1>(1);
}

inline constexpr auto acknowledgedTransmissionMode = UInt<1>(1);


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
