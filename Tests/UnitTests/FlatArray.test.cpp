#include <Sts1CobcSw/Utility/FlatArray.hpp>

// #include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <span>
#include <type_traits>


using sts1cobcsw::FlatArray;


namespace
{
struct S
{
    int i = 1;

    friend auto operator==(S const & lhs, S const & rhs) -> bool
    {
        return lhs.i == rhs.i;
    }
};
}


// We cannot test if FlatArray() of different types does not compile because the CallFlatArray trick
// does not work here. The reason is that we use a static_assert() in FlatArray() instead of a
// requires clause.
TEST_CASE("FlatArray() from values of the same type")
{
    CHECK(FlatArray(1) == std::array{1});
    CHECK(FlatArray(1U, 2U) == std::array{1U, 2U});
    CHECK(FlatArray(1L, 2L, 3L) == std::array{1L, 2L, 3L});
    CHECK(FlatArray(1.1, 2.2, 3.3, 4.4) == std::array{1.1, 2.2, 3.3, 4.4});
    CHECK(FlatArray(S(), S(), S()) == std::array{S(), S(), S()});
}


namespace
{
// Mix const and non-const arrays
constexpr auto a1 = std::array{1};
auto const a2 = std::array{2, 3};
auto a3 = std::array{4, 5, 6};
}


TEST_CASE("FlatArray() from arrays of the same type")
{
    // Single arrays with different lengths
    CHECK(FlatArray(a1) == a1);
    CHECK(FlatArray(a2) == a2);
    CHECK(FlatArray(a3) == a3);

    // Multiple arrays with same length
    CHECK(FlatArray(a1, a1) == std::array{1, 1});
    CHECK(FlatArray(a2, a2) == std::array{2, 3, 2, 3});
    CHECK(FlatArray(a3, a3) == std::array{4, 5, 6, 4, 5, 6});
    CHECK(FlatArray(a3, a3, a3) == std::array{4, 5, 6, 4, 5, 6, 4, 5, 6});

    // Multiple arrays with different lengths
    CHECK(FlatArray(a1, a2) == std::array{1, 2, 3});
    CHECK(FlatArray(a2, a3) == std::array{2, 3, 4, 5, 6});
    CHECK(FlatArray(a3, a1) == std::array{4, 5, 6, 1});
    CHECK(FlatArray(a1, a2, a3) == std::array{1, 2, 3, 4, 5, 6});
    CHECK(FlatArray(a2, a3, a1) == std::array{2, 3, 4, 5, 6, 1});
    CHECK(FlatArray(a3, a1, a2) == std::array{4, 5, 6, 1, 2, 3});
}


namespace
{
// Mix all constnesses of spans
auto s1 = std::span<int const, 1>{a1};
auto const s2 = std::span<int const, 2>{a2};
auto s3 = std::span<int, 3>{a3};
auto const s4 = std::span<int, 3>{a3};
}


TEST_CASE("FlatArray() from spans of the same type")
{
    // Single spans with different lengths
    CHECK(FlatArray(s1) == a1);
    CHECK(FlatArray(s2) == a2);
    CHECK(FlatArray(s3) == a3);
    CHECK(FlatArray(s4) == a3);

    // Multiple spans with same length
    CHECK(FlatArray(s1, s1) == std::array{1, 1});
    CHECK(FlatArray(s2, s2) == std::array{2, 3, 2, 3});
    CHECK(FlatArray(s3, s3) == std::array{4, 5, 6, 4, 5, 6});
    CHECK(FlatArray(s4, s4) == std::array{4, 5, 6, 4, 5, 6});
    CHECK(FlatArray(s3, s3, s3) == std::array{4, 5, 6, 4, 5, 6, 4, 5, 6});
}


TEST_CASE("FlatArray() from values, arrays, and spans of the same type")
{
    // Mix values and arrays
    CHECK(FlatArray(0, a1) == std::array{0, 1});
    CHECK(FlatArray(a2, 0) == std::array{2, 3, 0});
    CHECK(FlatArray(0, 0, a3) == std::array{0, 0, 4, 5, 6});
    CHECK(FlatArray(0, a2, 0) == std::array{0, 2, 3, 0});
    CHECK(FlatArray(a1, 0, a3) == std::array{1, 0, 4, 5, 6});

    // Mix values and spans
    CHECK(FlatArray(0, s1) == std::array{0, 1});
    CHECK(FlatArray(s2, 0) == std::array{2, 3, 0});
    CHECK(FlatArray(0, 0, s3) == std::array{0, 0, 4, 5, 6});
    CHECK(FlatArray(0, s2, 0) == std::array{0, 2, 3, 0});
    CHECK(FlatArray(s1, 0, s4) == std::array{1, 0, 4, 5, 6});

    // Mix values, arrays, and spans
    CHECK(FlatArray(0, a1, s1) == std::array{0, 1, 1});
    CHECK(FlatArray(a2, 0, s2) == std::array{2, 3, 0, 2, 3});
    CHECK(FlatArray(s3, a3, 0) == std::array{4, 5, 6, 4, 5, 6, 0});
    CHECK(FlatArray(s4, 0, a1) == std::array{4, 5, 6, 0, 1});
    CHECK(FlatArray(1, a1, s1, a2, s2, a3, s3)
          == std::array{1, 1, 1, 2, 3, 2, 3, 4, 5, 6, 4, 5, 6});
}
