#include <Sts1CobcSw/Periphery/PersistentState.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>


using sts1cobcsw::periphery::PersistentState;


TEST_CASE("PersistentState is non-default constructible, non-copyable and non-moveable")
{
    REQUIRE(not std::is_default_constructible_v<PersistentState<char>>);
    REQUIRE(not std::is_copy_constructible_v<PersistentState<short>>);
    REQUIRE(not std::is_copy_assignable_v<PersistentState<int>>);
    REQUIRE(not std::is_move_constructible_v<PersistentState<float>>);
    REQUIRE(not std::is_move_assignable_v<PersistentState<double>>);
}


TEST_CASE("Get() and Set()")
{
    PersistentState<int> persistentInt(13);
    REQUIRE(persistentInt.Get() == 13);
    persistentInt.Set(-273);
    REQUIRE(persistentInt.Get() == -273);
}
