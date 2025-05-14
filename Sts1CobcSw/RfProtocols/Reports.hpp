#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/MessageTypeIdFields.hpp>
#include <Sts1CobcSw/RfProtocols/Payload.hpp>
#include <Sts1CobcSw/RfProtocols/TmSpacePacketSecondaryHeader.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <etl/vector.h>

#include <bit>
#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
struct RequestId
{
    UInt<3> packetVersionNumber;
    UInt<1> packetType;
    UInt<1> secondaryHeaderFlag;
    Apid apid;
    UInt<2> sequenceFlags;
    UInt<14> packetSequenceCount;  // NOLINT(*magic-numbers)
};


// The values are chosen to match the subtype IDs of the failed verfication reports. The IDs of the
// successful ones are one less. The implementations of the reports rely on this.
enum class VerificationStage
{
    acceptance = 2,
    completionOfExecution = 8,
};


template<VerificationStage stage>
class SuccessfulVerificationReport : public Payload
{
public:
    static constexpr auto verificationStage = stage;

    explicit SuccessfulVerificationReport(RequestId const & requestId);


private:
    static constexpr auto messageTypeId =
        Make<tm::MessageTypeId, MessageTypeIdFields{1, static_cast<int>(stage) - 1}>();
    mutable tm::SpacePacketSecondaryHeader<messageTypeId> secondaryHeader_;
    RequestId requestId_;

    auto DoWriteTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};


template<VerificationStage stage>
class FailedVerificationReport : public Payload
{
public:
    static constexpr auto verificationStage = stage;

    FailedVerificationReport(RequestId const & requestId, ErrorCode errorCode);


private:
    static constexpr auto messageTypeId =
        Make<tm::MessageTypeId, MessageTypeIdFields{1, static_cast<int>(stage)}>();
    mutable tm::SpacePacketSecondaryHeader<messageTypeId> secondaryHeader_;
    RequestId requestId_;
    ErrorCode errorCode_;

    auto DoWriteTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};


template<>
inline constexpr std::size_t serialSize<RequestId> =
    totalSerialSize<decltype(RequestId::packetVersionNumber),
                    decltype(RequestId::packetType),
                    decltype(RequestId::secondaryHeaderFlag),
                    decltype(RequestId::apid.Value()),
                    decltype(RequestId::sequenceFlags),
                    decltype(RequestId::packetSequenceCount)>;


template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, RequestId const & requestId) -> void *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, RequestId & requestId) -> void const *;
}
