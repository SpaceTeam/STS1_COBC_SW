#include <Sts1CobcSw/Blake2s/Blake2s.hpp>

#include <Sts1CobcSw/Blake2s/External/BLAKE2s.h>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <bit>
#include <cstdint>


namespace sts1cobcsw::blake2s
{
using sts1cobcsw::operator""_b;


auto ComputeHash(std::span<Byte const> data) -> Hash
{
    static constexpr auto keySize = 4;
    // TODO: Use a proper key
    static constexpr std::uint32_t key = 0x0000'0000;
    // TODO: Check if little or big endian is the right choice here
    static auto const serializedKey = Serialize<std::endian::little>(key);
    static_assert(serializedKey.size() == keySize);

    auto blake2s = BLAKE2s();
    blake2s.reset(serializedKey.data(), serializedKey.size(), sizeof(Hash));
    blake2s.update(data.data(), data.size());
    auto hash = Hash{};
    blake2s.finalize(hash.data(), hash.size());
    return hash;
}
}
