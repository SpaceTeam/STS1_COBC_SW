#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <libfec/fixed.h>
// The macro FCR that is defined in libfec/fec.h clashes with a struct member name in a CMSIS
// header. Since we don't need the FCR macro, we can undefine it here to avoid the clash.
#undef FCR

#include <span>


namespace sts1cobcsw::rs
{
inline constexpr auto blockLength = NN;
inline constexpr auto nParitySymbols = NROOTS;
inline constexpr auto messageLength = NN - NROOTS;


auto Encode(std::span<Byte, messageLength> message, std::span<Byte, nParitySymbols> paritySymbols)
    -> void;
// Return the number of corrected errors
auto Decode(std::span<Byte, blockLength> block) -> Result<int>;
}
