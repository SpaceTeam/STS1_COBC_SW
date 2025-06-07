#include <Sts1CobcSw/Blake2s/Blake2s.hpp>

#include <Sts1CobcSw/Blake2s/External/BLAKE2s.h>


namespace sts1cobcsw::blake2s
{
using sts1cobcsw::operator""_b;


auto ComputeHash(std::span<Byte const> data) -> Hash
{
    // TODO: Choose a key, and find out how to store/set it securely
    static constexpr auto keySize = 4;
    static constexpr auto key = std::array{0x00_b, 0x00_b, 0x00_b, 0x00_b};
    static_assert(key.size() == keySize);
    auto blake2s = BLAKE2s();
    blake2s.reset(key.data(), key.size(), sizeof(Hash));
    blake2s.update(data.data(), data.size());
    auto hash = Hash{};
    blake2s.finalize(hash.data(), hash.size());
    return hash;
}
}
