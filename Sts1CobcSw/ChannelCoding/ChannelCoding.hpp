#pragma once


#include <Sts1CobcSw/ChannelCoding/ReedSolomon.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <span>


namespace sts1cobcsw
{
inline constexpr auto blockLength = rs::blockLength;
inline constexpr auto messageLength = rs::messageLength;
inline constexpr auto nParitySymbols = rs::nParitySymbols;


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
