#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw
{
namespace tc
{
// TODO: Add security header and trailer
struct TransferFrame
{
    struct PrimaryHeader
    {
        UInt<2> versionNumber;
        UInt<1> bypassFlag;
        UInt<1> controlCommandFlag;
        UInt<2> spare;
        SpacecraftId spacecraftId;
        UInt<3> extraVcidBits;
        Vcid vcid;
        UInt<10> frameLength;  // NOLINT(*magic-numbers)
        std::uint8_t frameSequenceNumber = 0;
    };

    PrimaryHeader primaryHeader;
    std::span<Byte const, transferFrameDataLength> dataField;
};


[[nodiscard]] auto ParseAsTransferFrame(std::span<Byte const, transferFrameLength> buffer)
    -> Result<TransferFrame>;

template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, TransferFrame::PrimaryHeader * header)
    -> void const *;
}


template<>
inline constexpr std::size_t serialSize<tc::TransferFrame::PrimaryHeader> =
    totalSerialSize<decltype(tc::TransferFrame::PrimaryHeader::versionNumber),
                    decltype(tc::TransferFrame::PrimaryHeader::bypassFlag),
                    decltype(tc::TransferFrame::PrimaryHeader::controlCommandFlag),
                    decltype(tc::TransferFrame::PrimaryHeader::spare),
                    decltype(tc::TransferFrame::PrimaryHeader::spacecraftId.Value()),
                    decltype(tc::TransferFrame::PrimaryHeader::extraVcidBits),
                    decltype(tc::TransferFrame::PrimaryHeader::vcid.Value()),
                    decltype(tc::TransferFrame::PrimaryHeader::frameLength)>
    + totalSerialSize<decltype(tc::TransferFrame::PrimaryHeader::frameSequenceNumber)>;
static_assert(serialSize<tc::TransferFrame::PrimaryHeader> == tc::transferFramePrimaryHeaderLength);
}
