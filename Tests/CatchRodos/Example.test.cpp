#include <Tests/CatchRodos/TestMacros.hpp>


TEST_INIT("Initialize something")
{
    // Someting to initialize
}


TEST_CASE("Correct arithmetic")
{
    CHECK(1 + 2 == 3);
    CHECK(5 - 6 == -1);
    CHECK(3 * 4 == 12);
    CHECK(24 / 6 == 4);
}


TEST_CASE("Wrong arithmetic")
{
    auto a = 10;
    auto b = 20;
    CHECK(a + b != 30);
    CHECK(a + b < 1);
    CHECK(a * b <= 2);
    REQUIRE(a - b >= 3);
    REQUIRE(a / b > 4);
}


TEST_CASE("Always passes")
{
    CHECK(1 == 1);
}
