#include <Sts1CobcSw/Periphery/FramMock.hpp>
#include <Sts1CobcSw/PersistentState/PersistentVariable.hpp>

#include <catch2/catch_test_macros.hpp>

void InitializeFramMocks()
{
    sts1cobcsw::fram::ram::SetAllDoFunctions();
}

TEST_CASE("Int test")
{
    InitializeFramMocks();
    auto variable = sts1cobcsw::PersistentVariable<int, 0x1000, 4>();
    variable.Store(2);
    auto result = variable.Load();
    CHECK(result == 2);
}
TEST_CASE("Float test")
{
    auto variable = sts1cobcsw::PersistentVariable<float, 0x2000, 4>();
    variable.Store(3.5F);
    auto result = variable.Load();
    CHECK(result == 3.5F);
}
TEST_CASE("Bool test")
{
    auto variable = sts1cobcsw::PersistentVariable<bool, 0x3000, 1>();
    variable.Store(bool{true});
    auto result = variable.Load();
    CHECK(result == true);
}
TEST_CASE("User-defined struct test")
{
    struct UserDefined
    {
        int a;
        float b;
    };

    auto variable = sts1cobcsw::PersistentVariable<UserDefined, 0x4000, 8>();
    UserDefined data{42, 3.14F};
    variable.Store(data);
    auto result = variable.Load();
    CHECK(result.a == data.a && result.b == data.b);
}
