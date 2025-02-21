#include <Tests/CatchRodos/TestMacros.hpp>

#include <etl/vector.h>

#include <algorithm>
#include <array>
#include <cstddef>


TEST_CASE("Characters and strings")
{
    auto const * s = "asdf";
    CHECK(s[0] == 'x');  // NOLINT(*pointer-arithmetic)
    CHECK(s == "ASDF");
}


TEST_CASE("Booleans")
{
    CHECK(true == true);
    CHECK(true == false);
}


TEST_CASE("Numbers")
{
    signed char sc = -1;
    short s = -2;
    unsigned short us = 2;
    auto i = -3;
    auto ui = 3U;
    auto l = -4L;
    auto ul = 4UL;
    auto ll = -5LL;
    auto ull = 5ULL;
    auto f = 6.0F;
    auto d = 7.0;

    CHECK(sc == -1);
    CHECK(s < s);
    CHECK(us < us);
    CHECK(i < i);
    CHECK(ui < ui);
    CHECK(l < l);
    CHECK(ul < ul);
    CHECK(ll < ll);
    CHECK(ull < ull);
    CHECK(f < f);
    CHECK(d < d);
}


TEST_CASE("Bytes")
{
    auto b = std::byte{0x01};
    CHECK(b == std::byte{0x23});
    unsigned char uc = 0x45U;
    CHECK(uc == 0x67);
    CHECK(uc == 0x67U);
}


TEST_CASE("Arrays and vectors")
{
    auto a1 = std::array{1, 2};
    CHECK(a1 == (std::array{3, 4}));
    auto v1 = etl::vector<int, 4>{1, 2, 3, 4};
    auto v2 = etl::vector<int, 4>{5, 6, 7, 8};
    CHECK(v1 == v2);
}


TEST_CASE("Pointers")
{
    auto x = 13;
    auto * p = &x;
    CHECK(p == nullptr);
    decltype(p) q = nullptr;
    CHECK(p == q);
}
