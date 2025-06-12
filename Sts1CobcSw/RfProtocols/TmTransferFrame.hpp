#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/IdCounters.hpp>
#include <Sts1CobcSw/RfProtocols/Payload.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <etl/vector.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw
{
namespace tm
{
// This implementation of the TM Space Data Link Protocol (CCSDS 132.0-B-3) assumes unsegmented
// packets and hard codes a lot of things.
class TransferFrame
{
public:
    struct PrimaryHeader
    {
        static constexpr auto versionNumber = transferFrameVersionNumber;
        static constexpr auto spacecraftId = sts1cobcsw::spacecraftId;
        Vcid vcid;
        static constexpr auto operationalControlFlag = UInt<1>(0);
        std::uint8_t masterChannelFrameCount = 0;
        std::uint8_t virtualChannelFrameCount = 0;
        static constexpr auto secondaryHeaderFlag = UInt<1>(0);
        static constexpr auto synchronizationFlag = UInt<1>(0);
        static constexpr auto packetOrderFlag = UInt<1>(0);
        static constexpr auto segmentLengthId = UInt<2>(0b11);
        static constexpr auto firstHeaderPointer = UInt<11>(0);
    };

    explicit TransferFrame(std::span<Byte, transferFrameLength> buffer);

    auto StartNew(Vcid vcid) -> void;
    [[nodiscard]] auto GetDataField() -> etl::vector_ext<Byte> &;
    [[nodiscard]] auto Add(Payload const & payload) -> Result<void>;
    auto Finish() -> void;


private:
    static inline auto virtualChannelFrameCounters = IdCounters<std::uint8_t, Vcid>{};
    // MCID = TF version No. + spacecraft ID, and TF version No. is a constant
    static inline auto masterChannelFrameCounters = IdCounters<std::uint8_t, SpacecraftId>{};

    PrimaryHeader primaryHeader_;
    std::span<Byte, transferFrameLength> buffer_;
    etl::vector_ext<Byte> dataField_ = etl::vector_ext<Byte>(
        buffer_.data() + transferFramePrimaryHeaderLength, transferFrameDataLength);
};


template<std::endian endianness>
[[nodiscard]] [[nodiscard]] auto SerializeTo(void * destination,
                                             TransferFrame::PrimaryHeader const & header) -> void *;
}


template<>
inline constexpr std::size_t serialSize<tm::TransferFrame::PrimaryHeader> =
    totalSerialSize<decltype(tm::TransferFrame::PrimaryHeader::versionNumber),
                    decltype(tm::TransferFrame::PrimaryHeader::spacecraftId.Value()),
                    decltype(tm::TransferFrame::PrimaryHeader::vcid.Value()),
                    decltype(tm::TransferFrame::PrimaryHeader::operationalControlFlag)>
    + totalSerialSize<decltype(tm::TransferFrame::PrimaryHeader::masterChannelFrameCount),
                      decltype(tm::TransferFrame::PrimaryHeader::virtualChannelFrameCount)>
    + totalSerialSize<decltype(tm::TransferFrame::PrimaryHeader::secondaryHeaderFlag),
                      decltype(tm::TransferFrame::PrimaryHeader::synchronizationFlag),
                      decltype(tm::TransferFrame::PrimaryHeader::packetOrderFlag),
                      decltype(tm::TransferFrame::PrimaryHeader::segmentLengthId),
                      decltype(tm::TransferFrame::PrimaryHeader::firstHeaderPointer)>;
static_assert(totalSerialSize<tm::TransferFrame::PrimaryHeader>
              == tm::transferFramePrimaryHeaderLength);
}
