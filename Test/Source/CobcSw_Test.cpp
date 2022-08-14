#include "Lib.hpp"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("Always passes")
{
    REQUIRE(true);
}


TEST_CASE("Library is called 'CobcSw'")
{
    const auto lib = cobc::Library();

    const auto nameIsCorrect = lib.name == "CobcSw";
    REQUIRE(nameIsCorrect);
}
