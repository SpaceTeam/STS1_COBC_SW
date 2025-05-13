#include <catch2/catch_test_macros.hpp>

#include <Sts1CobcSw/Blake2s/BLAKE2s.h>

#include <array>


TEST_CASE("Blake2s")  // NOLINT(cert-err58-cpp)
{
    auto blake = BLAKE2s();
    static constexpr auto key = 0;
    static constexpr auto outputLength = 8;
    static_assert(sizeof(long long) == outputLength);
    auto data = std::to_array<std::uint8_t>({0});
    // char data[] = "12345678";
    blake.reset(&key, sizeof(key), outputLength);
    blake.update(data.data(), data.size());
    //blake.update(&data[0], sizeof(data));
    auto hash = 0ULL;
    blake.finalize(&hash, outputLength);
    CHECK(hash == 0x1550c7704ecd25a2);
}
