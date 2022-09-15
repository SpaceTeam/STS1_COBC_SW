#include <Sts1CobcSw/Dummy.hpp>

#include <catch2/catch_test_macros.hpp>


TEST_CASE("Always passes")
{
    REQUIRE(true);
}


TEST_CASE("Dummy is called 'Sts1CobcSw'")
{
    const auto dummy = sts1cobcsw::Dummy();

    const auto nameIsCorrect = dummy.name == "Sts1CobcSw";
    REQUIRE(nameIsCorrect);
}
