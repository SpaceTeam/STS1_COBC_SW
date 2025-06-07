#pragma once


#include <Sts1CobcSw/ChannelCoding/ReedSolomon.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <span>


namespace sts1cobcsw
{
inline constexpr auto blockLength = rs::blockLength;
inline constexpr auto messageLength = rs::messageLength;
inline constexpr auto nParitySymbols = rs::nParitySymbols;


// TODO: Change to a single parameter of blockLength
auto Encode(std::span<Byte, blockLength> block) -> void;
auto Decode(std::span<Byte, blockLength> block) -> void;
}
