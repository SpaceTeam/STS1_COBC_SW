#include <Sts1CobcSw/Dummy.hpp>

#include <catch2/catch_test_macros.hpp>

#include <etl/string.h>


TEST_CASE("Always passes")
{
    REQUIRE(true);
}


TEST_CASE("Dummy is called 'Sts1CobcSw'")
{
    auto const dummy = sts1cobcsw::Dummy();

    auto const nameIsCorrect = dummy.name == "Sts1CobcSw";
    REQUIRE(nameIsCorrect);
}
