#include <Sts1CobcSw/ChannelCoding/ChannelCoding.hpp>

#ifndef DISABLE_CHANNEL_CODING
    #include <Sts1CobcSw/ChannelCoding/ReedSolomon.hpp>
    #include <Sts1CobcSw/ChannelCoding/Scrambler.hpp>
#endif
#include <Sts1CobcSw/Outcome/Outcome.hpp>


namespace sts1cobcsw
{
namespace tm
{
auto Encode(std::span<Byte, blockLength> block) -> void
{
#ifdef DISABLE_CHANNEL_CODING
    (void)block;
#else
    rs::Encode(block.first<rs::messageLength>(), block.last<rs::nParitySymbols>());
    Scramble(block);
#endif
}


auto Decode(std::span<Byte, blockLength> block) -> Result<int>
{
#ifdef DISABLE_CHANNEL_CODING
    (void)block;
    return 0;
#else
    Unscramble(block);
    return rs::Decode(block);
#endif
}
}


namespace tc
{
auto Encode(std::span<Byte, blockLength> block) -> void
{
#ifdef DISABLE_CHANNEL_CODING
    (void)block;
#else
    rs::Encode(block.first<rs::messageLength>(), block.last<rs::nParitySymbols>());
    Scramble(block);
#endif
}


auto Decode(std::span<Byte, blockLength> block) -> Result<int>
{
#ifdef DISABLE_CHANNEL_CODING
    (void)block;
    return 0;
#else
    Unscramble(block);
    return rs::Decode(block);
#endif
}
}
}
