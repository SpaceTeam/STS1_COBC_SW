#pragma once


#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <bit>
#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
struct ProtocolDataUnitHeader
{
    UInt<3> version;
    UInt<1> pduType;
    UInt<1> direction;
    UInt<1> transmissionMode;
    UInt<1> crcFlag;
    UInt<1> largeFileFlag;
    std::uint16_t pduDataFieldLength = 0;
    UInt<1> segmentationControl;
    UInt<3> lengthOfEntityIds;
    UInt<1> segmentMetadataFlag;
    UInt<3> lengthOfTransactionSequenceNumber;
    EntityId sourceEntityId;
    std::uint16_t transactionSequenceNumber = 0;
    EntityId destinationEntityId;
};


template<>
inline constexpr std::size_t serialSize<ProtocolDataUnitHeader> =
    totalSerialSize<decltype(ProtocolDataUnitHeader::version),
                    decltype(ProtocolDataUnitHeader::pduType),
                    decltype(ProtocolDataUnitHeader::direction),
                    decltype(ProtocolDataUnitHeader::transmissionMode),
                    decltype(ProtocolDataUnitHeader::crcFlag),
                    decltype(ProtocolDataUnitHeader::largeFileFlag)>
    + totalSerialSize<decltype(ProtocolDataUnitHeader::pduDataFieldLength)>
    + totalSerialSize<decltype(ProtocolDataUnitHeader::segmentationControl),
                      decltype(ProtocolDataUnitHeader::lengthOfEntityIds),
                      decltype(ProtocolDataUnitHeader::segmentMetadataFlag),
                      decltype(ProtocolDataUnitHeader::lengthOfTransactionSequenceNumber)>
    + totalSerialSize<decltype(ProtocolDataUnitHeader::sourceEntityId),
                      decltype(ProtocolDataUnitHeader::transactionSequenceNumber),
                      decltype(ProtocolDataUnitHeader::destinationEntityId)>;
static_assert(serialSize<ProtocolDataUnitHeader> == tm::pduHeaderLength);
static_assert(serialSize<ProtocolDataUnitHeader> == tc::pduHeaderLength);


template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, ProtocolDataUnitHeader const & header) -> void *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, ProtocolDataUnitHeader * header)
    -> void const *;
}
