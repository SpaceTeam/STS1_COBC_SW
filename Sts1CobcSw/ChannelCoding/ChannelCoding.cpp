#include <Sts1CobcSw/ChannelCoding/ChannelCoding.hpp>
#include <Sts1CobcSw/ChannelCoding/ReedSolomon.hpp>
#include <Sts1CobcSw/ChannelCoding/Scrambler.hpp>


namespace sts1cobcsw
{
namespace tm
{
auto Encode(std::span<Byte, blockLength> block) -> void
{
    rs::Encode(block.first<rs::messageLength>(), block.last<rs::nParitySymbols>());
    Scramble(block);
}


auto Decode(std::span<Byte, blockLength> block) -> void
{
    Unscramble(block);
    rs::Decode(block);
}
}


namespace tc
{
auto Encode(std::span<Byte, blockLength> block) -> void
{
    rs::Encode(block.first<rs::messageLength>(), block.last<rs::nParitySymbols>());
    Scramble(block);
}


auto Decode(std::span<Byte, blockLength> block) -> void
{
    Unscramble(block);
    rs::Decode(block);
}
}
}
