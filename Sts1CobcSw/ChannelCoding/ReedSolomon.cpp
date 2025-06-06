#include "ReedSolomon.hpp"
extern "C"
{
#include <libfec/fec.h>
}

namespace sts1cobcsw::rs
{
auto Encode(std::span<Byte, messageLength> data, std::span<Byte, nParitySymbols> parities) -> void
{
    encode_rs_ccsds(
        std::span<unsigned char>(reinterpret_cast<unsigned char *>(data.data()), data.size())
            .data(),
        std::span<unsigned char>(reinterpret_cast<unsigned char *>(parities.data()),
                                 parities.size())
            .data());
}
auto Decode(std::span<Byte, blockLength> data) -> void
{
    decode_rs_ccsds(
        std::span<unsigned char>(reinterpret_cast<unsigned char *>(data.data()), data.size())
            .data());
}
}
