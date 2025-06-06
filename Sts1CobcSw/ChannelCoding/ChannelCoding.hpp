#pragma once

#include <span>

#include "../Serial/Byte.hpp"
#include "Configuration.hpp"

namespace sts1cobcsw
{
auto Encode(std::span<Byte, messageLength> data, std::span<Byte, nParitySymbols> parities) -> void;
auto Decode(std::span<Byte, blockLength> data) -> void;
}
