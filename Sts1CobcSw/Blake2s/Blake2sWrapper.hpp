#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <array>
#include <span>


namespace sts1cobcsw::blake2s
{
using Hash = std::array<Byte, 8>;  // NOLINT(*magic-numbers)


auto ComputeHash(std::span<Byte const> data) -> Hash;
}
