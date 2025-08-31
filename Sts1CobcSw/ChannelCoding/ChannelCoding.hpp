#pragma once


#include <Sts1CobcSw/ChannelCoding/External/ConvolutionalCoding.hpp>
#include <Sts1CobcSw/ChannelCoding/ReedSolomon.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <array>
#include <span>


namespace sts1cobcsw
{
using sts1cobcsw::operator""_b;


inline constexpr auto attachedSynchMarker = std::array{0x1A_b, 0xCF_b, 0xFC_b, 0x1D_b};
#ifdef DISABLE_CHANNEL_CODING
inline constexpr auto blockLength = rs::messageLength;
inline constexpr auto attachedSynchMarkerLength = 0;
#else
inline constexpr auto blockLength = rs::blockLength;
inline constexpr auto attachedSynchMarkerLength = attachedSynchMarker.size();
#endif
inline constexpr auto messageLength = rs::messageLength;
inline constexpr auto nParitySymbols = rs::nParitySymbols;
inline constexpr auto channelAccessDataUnitLength = attachedSynchMarkerLength + blockLength;
inline constexpr auto fullyEncodedFrameLength = static_cast<unsigned>(
    cc::ViterbiCodec::EncodedSize(channelAccessDataUnitLength, /*withFlushBits=*/true));


namespace tc
{
auto Encode(std::span<Byte, blockLength> block) -> void;
// Return the number of corrected errors
auto Decode(std::span<Byte, blockLength> block) -> Result<int>;
}

namespace tm
{
auto Encode(std::span<Byte, blockLength> block) -> void;
// Return the number of corrected errors
auto Decode(std::span<Byte, blockLength> block) -> Result<int>;
}
}
