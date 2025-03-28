#include <Sts1CobcSw/FramSections/PersistentVariableInfo.hpp>
#include <Sts1CobcSw/FramSections/SubsectionInfo.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/StringLiteral.hpp>

#include <catch2/catch_test_macros.hpp>


using sts1cobcsw::APersistentVariableInfo;
using sts1cobcsw::isAPersistentVariableInfo;
using sts1cobcsw::PersistentVariableInfo;
using sts1cobcsw::SubsectionInfoLike;


TEST_CASE("All static_asserts passed")
{
    CHECK(true);
}


struct MyStruct
{
    short s;
};


template<>
inline constexpr auto sts1cobcsw::serialSize<MyStruct> =
    sts1cobcsw::totalSerialSize<decltype(MyStruct::s)>;


constexpr auto intInfo = PersistentVariableInfo<"i", int>();
constexpr auto structInfo = PersistentVariableInfo<"myStruct", MyStruct>();

static_assert(isAPersistentVariableInfo<decltype(intInfo)>);
static_assert(isAPersistentVariableInfo<decltype(structInfo)>);
static_assert(not isAPersistentVariableInfo<int>);
static_assert(not isAPersistentVariableInfo<MyStruct>);

static_assert(APersistentVariableInfo<decltype(intInfo)>);
static_assert(APersistentVariableInfo<decltype(structInfo)>);
static_assert(not APersistentVariableInfo<double>);
static_assert(not APersistentVariableInfo<MyStruct>);

static_assert(SubsectionInfoLike<decltype(intInfo)>);
static_assert(SubsectionInfoLike<decltype(structInfo)>);
static_assert(not SubsectionInfoLike<bool>);
static_assert(not SubsectionInfoLike<MyStruct>);
