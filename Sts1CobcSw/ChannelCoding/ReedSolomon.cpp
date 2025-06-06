#include <Sts1CobcSw/ChannelCoding/ReedSolomon.hpp>

#include <libfec/fec.h>


namespace sts1cobcsw::rs
{
auto Encode(std::span<Byte, messageLength> message, std::span<Byte, nParitySymbols> paritySymbols)
    -> void
{
    // NOLINTBEGIN(*reinterpret-cast)
    encode_rs_ccsds(reinterpret_cast<unsigned char *>(message.data()),
                    reinterpret_cast<unsigned char *>(paritySymbols.data()));
    // NOLINTEND(*reinterpret-cast)
}


auto Decode(std::span<Byte, blockLength> block) -> Result<int>
{
    // NOLINTNEXTLINE(*reinterpret-cast)
    auto nCorrectedErrors = decode_rs_ccsds(reinterpret_cast<unsigned char *>(block.data()));
    if(nCorrectedErrors < 0)
    {
        return ErrorCode::errorCorrectionFailed;
    }
    return nCorrectedErrors;
}
}
