#include <Sts1CobcSw/ChannelCoding/ChannelCoding.hpp>

#include <Sts1CobcSw/ChannelCoding/ReedSolomon.hpp>
#include <Sts1CobcSw/ChannelCoding/Scrambler.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>


namespace sts1cobcsw
{
namespace tm
{
auto Encode(std::span<Byte, blockLength> block) -> void
{
    rs::Encode(block.first<rs::messageLength>(), block.last<rs::nParitySymbols>());
    Scramble(block);
}


auto Decode(std::span<Byte, blockLength> block) -> Result<int>
{
    Unscramble(block);
    return rs::Decode(block);
}
}


namespace tc
{
auto Encode(std::span<Byte, blockLength> block) -> void
{
    rs::Encode(block.first<rs::messageLength>(), block.last<rs::nParitySymbols>());
    Scramble(block);
}


auto Decode(std::span<Byte, blockLength> block) -> Result<int>
{
    Unscramble(block);
    return rs::Decode(block);
}
}
}
