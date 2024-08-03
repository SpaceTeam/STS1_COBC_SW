#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramMock.hpp>
#include <Sts1CobcSw/PersistentState/PersistentVariable.hpp>

#include <catch2/catch_test_macros.hpp>
#include <strong_type/type.hpp>


using sts1cobcsw::PersistentVariable;
using sts1cobcsw::fram::Address;
using sts1cobcsw::fram::Size;


void InitializeFramMocks()
{
    sts1cobcsw::fram::ram::SetAllDoFunctions();
}


TEST_CASE("Int test")
{
    InitializeFramMocks();
    auto variable = PersistentVariable<int, Address(0x1000), Size(4)>();
    variable.Store(2);
    auto result = variable.Load();
    CHECK(result == 2);
}


TEST_CASE("Float test")
{
    InitializeFramMocks();
    auto variable = PersistentVariable<float, Address(0x2000), Size(4)>();
    variable.Store(3.5F);
    auto result = variable.Load();
    CHECK(result == 3.5F);
}


TEST_CASE("Bool test")
{
    InitializeFramMocks();
    auto variable = PersistentVariable<bool, Address(0x3000), Size(1)>();
    variable.Store(bool{true});
    auto result = variable.Load();
    CHECK(result == true);
}


struct UserDefined
{
    int a;
    float b;
    friend constexpr auto operator==(UserDefined const & lhs, UserDefined const & rhs) -> bool;
};


constexpr auto operator==(UserDefined const & lhs, UserDefined const & rhs) -> bool
{
    return lhs.a == rhs.a && lhs.b == rhs.b;
}


TEST_CASE("User-defined struct test")
{
    InitializeFramMocks();
    auto variable = PersistentVariable<UserDefined, Address(0x4000), Size(8)>();
    UserDefined data{42, 3.14F};
    variable.Store(data);
    auto result = variable.Load();
    CHECK(result.a == data.a);
    CHECK(result.b == data.b);
}
