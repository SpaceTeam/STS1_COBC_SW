#include <Sts1CobcSw/Utility/StringLiteral.hpp>

#include <catch2/catch_test_macros.hpp>


using sts1cobcsw::StringLiteral;


TEST_CASE("All static_asserts passed")
{
    REQUIRE(true);
}


static_assert(StringLiteral("") == StringLiteral(""));
static_assert(StringLiteral("one") == StringLiteral("one"));
static_assert(StringLiteral("one") != StringLiteral("two"));
static_assert(StringLiteral("one") != StringLiteral("three"));


template<StringLiteral personName>
struct Person
{
    static constexpr auto name = personName;
};


static_assert(Person<"John">::name == StringLiteral("John"));
static_assert(Person<"John">::name != Person<"Fred">::name);
static_assert(Person<"John">::name != Person<"Samantha">::name);
