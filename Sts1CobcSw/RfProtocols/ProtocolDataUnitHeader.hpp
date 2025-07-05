#pragma once


#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <strong_type/regular.hpp>
#include <strong_type/type.hpp>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>


namespace sts1cobcsw
{
using PduType = strong::type<UInt<1>, struct PduTypeTag, strong::regular>;
using Direction = strong::type<UInt<1>, struct DirectionTag, strong::regular>;
using TransmissionMode = strong::type<UInt<1>, struct TransmissionModeTag, strong::regular>;


struct ProtocolDataUnitHeader
{
    UInt<3> version;
    PduType pduType;
    Direction direction;
    TransmissionMode transmissionMode;
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


inline constexpr auto fileDirectivePduType = PduType(0);
inline constexpr auto fileDataPduType = PduType(1);

inline constexpr auto towardsFileReceiverDirection = Direction(0);
inline constexpr auto towardsFileSenderDirection = Direction(1);

inline constexpr auto acknowledgedTransmissionMode = TransmissionMode(1);


template<>
inline constexpr std::size_t serialSize<ProtocolDataUnitHeader> =
    totalSerialSize<decltype(ProtocolDataUnitHeader::version),
                    strong::underlying_type_t<PduType>,
                    strong::underlying_type_t<Direction>,
                    strong::underlying_type_t<TransmissionMode>,
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
