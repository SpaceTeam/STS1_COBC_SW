#include <Sts1CobcSw/FramSections/PersistentVariableInfo.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramMock.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/StringLiteral.hpp>

#include <catch2/catch_test_macros.hpp>
#include <strong_type/type.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <type_traits>


using sts1cobcsw::PersistentVariableInfo;
using sts1cobcsw::PersistentVariables;
using sts1cobcsw::Section;
using sts1cobcsw::fram::Address;
using sts1cobcsw::fram::Size;


TEST_CASE("All static_asserts passed")
{
    REQUIRE(true);
}


constexpr auto section0 = Section<Address(0), Size(100)>();
constexpr auto section1 = Section<Address(100), Size(100)>();
constexpr auto section2 = Section<Address(200), Size(100)>();

constexpr auto pvs = PersistentVariables<section0,
                                         section1,
                                         section2,
                                         PersistentVariableInfo<"nResets", std::uint32_t>,
                                         PersistentVariableInfo<"activeFwImage", std::uint8_t>,
                                         PersistentVariableInfo<"somethingElse", std::int16_t>>();

static_assert(std::is_same_v<decltype(pvs)::ValueType<"nResets">, std::uint32_t>);
static_assert(std::is_same_v<decltype(pvs)::ValueType<"activeFwImage">, std::uint8_t>);
static_assert(std::is_same_v<decltype(pvs)::ValueType<"somethingElse">, std::int16_t>);

static_assert(std::is_same_v<decltype(pvs.Load<"nResets">()), std::uint32_t>);
static_assert(std::is_same_v<decltype(pvs.Load<"activeFwImage">()), std::uint8_t>);
static_assert(std::is_same_v<decltype(pvs.Load<"somethingElse">()), std::int16_t>);


TEST_CASE("Majority vote")
{
    using sts1cobcsw::ComputeMajorityVote;

    auto voteResult = ComputeMajorityVote(173, 173, 173);
    CHECK(voteResult.has_value());
    CHECK(voteResult.value() == 173);  // NOLINT(*unchecked-optional-access)

    voteResult = ComputeMajorityVote(-2, 173, 173);
    CHECK(voteResult.has_value());
    CHECK(voteResult.value() == 173);  // NOLINT(*unchecked-optional-access)

    voteResult = ComputeMajorityVote(173, -2, 173);
    CHECK(voteResult.has_value());
    CHECK(voteResult.value() == 173);  // NOLINT(*unchecked-optional-access)

    voteResult = ComputeMajorityVote(173, 173, -2);
    CHECK(voteResult.has_value());
    CHECK(voteResult.value() == 173);  // NOLINT(*unchecked-optional-access)

    voteResult = ComputeMajorityVote(17, 173, -2);
    CHECK(not voteResult.has_value());
}


TEST_CASE("Load and store")
{
    namespace fram = sts1cobcsw::fram;
    using fram::Address;
    using fram::ram::memory;
    using sts1cobcsw::operator""_b;

    fram::ram::SetAllDoFunctions();
    memory.fill(0x00_b);

    auto nResets = pvs.Load<"nResets">();
    auto activeFwImage = pvs.Load<"activeFwImage">();
    auto somethingElse = pvs.Load<"somethingElse">();

    CHECK(nResets == 0);
    CHECK(activeFwImage == 0);
    CHECK(somethingElse == 0);

    pvs.Store<"nResets">(0x12345678);
    pvs.Store<"activeFwImage">(42);
    pvs.Store<"somethingElse">(-2);

    nResets = pvs.Load<"nResets">();
    activeFwImage = pvs.Load<"activeFwImage">();
    somethingElse = pvs.Load<"somethingElse">();

    CHECK(nResets == 0x12345678);
    CHECK(activeFwImage == 42);
    CHECK(somethingElse == -2);

    constexpr auto nResetsAddress0 = 0;
    constexpr auto activeFwImageAddress0 = 4;
    constexpr auto somethingElseAddress0 = 5;
    constexpr auto nResetsAddress1 = nResetsAddress0 + 100;
    constexpr auto nResetsAddress2 = nResetsAddress1 + 100;

    CHECK(memory[nResetsAddress0] == 0x78_b);
    CHECK(memory[nResetsAddress0 + 1] == 0x56_b);
    CHECK(memory[nResetsAddress0 + 2] == 0x34_b);
    CHECK(memory[nResetsAddress0 + 3] == 0x12_b);
    CHECK(memory[activeFwImageAddress0] == 42_b);
    CHECK(memory[somethingElseAddress0] == 0xFE_b);
    CHECK(memory[somethingElseAddress0 + 1] == 0xFF_b);

    memory[nResetsAddress0 + 3] = 0x00_b;
    memory[activeFwImageAddress0] = 0x00_b;
    memory[somethingElseAddress0 + 1] = 0x00_b;

    nResets = pvs.Load<"nResets">();
    activeFwImage = pvs.Load<"activeFwImage">();
    somethingElse = pvs.Load<"somethingElse">();

    CHECK(nResets == 0x12345678);
    CHECK(activeFwImage == 42);
    CHECK(somethingElse == -2);

    CHECK(memory[nResetsAddress0] == 0x78_b);
    CHECK(memory[nResetsAddress0 + 1] == 0x56_b);
    CHECK(memory[nResetsAddress0 + 2] == 0x34_b);
    CHECK(memory[nResetsAddress0 + 3] == 0x12_b);
    CHECK(memory[activeFwImageAddress0] == 42_b);
    CHECK(memory[somethingElseAddress0] == 0xFE_b);
    CHECK(memory[somethingElseAddress0 + 1] == 0xFF_b);

    memory[nResetsAddress1 + 1] = 0x00_b;
    nResets = pvs.Load<"nResets">();
    CHECK(nResets == 0x12345678);
    CHECK(memory[nResetsAddress0] == 0x78_b);

    memory[nResetsAddress2 + 2] = 0x00_b;
    nResets = pvs.Load<"nResets">();
    CHECK(nResets == 0x12345678);
    CHECK(memory[nResetsAddress0] == 0x78_b);

    // TODO: Add more tests, especially for the case when the values are not equal
}
