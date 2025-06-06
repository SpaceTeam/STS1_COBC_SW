#include "ChannelCoding.hpp"

#include <array>
#include <cstring>

#include "ReedSolomon.hpp"
#include "Scrambler.hpp"

namespace sts1cobcsw
{
auto Encode(std::span<Byte, messageLength> data, std::span<Byte, nParitySymbols> parities) -> void
{
    rs::Encode(data, parities);
    std::array<Byte, messageLength + nParitySymbols> buffer = {};
    std::memcpy(buffer.data(), data.data(), messageLength);
    std::memcpy(buffer.data() + messageLength, parities.data(), nParitySymbols);
    Scramble(buffer);
    std::memcpy(data.data(), buffer.data(), messageLength);
    std::memcpy(parities.data(), buffer.data() + messageLength, nParitySymbols);
}

auto Decode(std::span<Byte, blockLength> data) -> void
{
    Unscramble(data);
    rs::Decode(data);
}
}
