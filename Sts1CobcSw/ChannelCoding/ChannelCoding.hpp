#pragma once


#include <Sts1CobcSw/ChannelCoding/ReedSolomon.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <span>


namespace sts1cobcsw
{
#ifdef DISABLE_CHANNEL_CODING
inline constexpr auto blockLength = rs::messageLength;
#else
inline constexpr auto blockLength = rs::blockLength;
#endif
inline constexpr auto messageLength = rs::messageLength;
inline constexpr auto nParitySymbols = rs::nParitySymbols;
// The factor of 1.5 comes from convolutional coding, the +1 is for rounding up
inline constexpr auto fullyEncodedFrameLength = static_cast<unsigned>(blockLength * 1.5 + 1);


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
