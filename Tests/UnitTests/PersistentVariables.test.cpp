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
#include <string>
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


TEST_CASE("Load() and Store()")
{
    using sts1cobcsw::fram::Address;
    using sts1cobcsw::fram::framIsWorking;
    using sts1cobcsw::fram::ram::memory;
    using sts1cobcsw::operator""_b;

    constexpr auto nResetsAddress0 = 0;
    constexpr auto activeFwImageAddress0 = 4;
    constexpr auto somethingElseAddress0 = 5;
    constexpr auto activeFwImageAddress1 = activeFwImageAddress0 + 100;
    constexpr auto activeFwImageAddress2 = activeFwImageAddress1 + 100;

    sts1cobcsw::fram::ram::SetAllDoFunctions();

    SECTION("FRAM is working")
    {
        memory.fill(0x00_b);
        framIsWorking = true;

        SECTION("You load what you store")
        {
            pvs.Store<"nResets">(0x12345678);
            pvs.Store<"activeFwImage">(42);
            pvs.Store<"somethingElse">(-2);
            CHECK(pvs.Load<"nResets">() == 0x12345678);
            CHECK(pvs.Load<"activeFwImage">() == 42);
            CHECK(pvs.Load<"somethingElse">() == -2);
        }

        SECTION("Store() writes to memory")
        {
            pvs.Store<"nResets">(0x12345678);
            pvs.Store<"activeFwImage">(42);
            pvs.Store<"somethingElse">(-2);
            CHECK(memory[nResetsAddress0] == 0x78_b);
            CHECK(memory[nResetsAddress0 + 1] == 0x56_b);
            CHECK(memory[nResetsAddress0 + 2] == 0x34_b);
            CHECK(memory[nResetsAddress0 + 3] == 0x12_b);
            CHECK(memory[activeFwImageAddress0] == 42_b);
            CHECK(memory[somethingElseAddress0] == 0xFE_b);
            CHECK(memory[somethingElseAddress0 + 1] == 0xFF_b);
        }

        SECTION("Load() reads from memory")
        {
            memory[activeFwImageAddress0] = 17_b;
            memory[activeFwImageAddress1] = 17_b;
            memory[activeFwImageAddress2] = 17_b;
            CHECK(pvs.Load<"activeFwImage">() == 17);
        }

        SECTION("Load() returns majority and repairs memory")
        {
            memory[activeFwImageAddress0] = 42_b;
            memory[activeFwImageAddress1] = 17_b;
            memory[activeFwImageAddress2] = 17_b;
            CHECK(pvs.Load<"activeFwImage">() == 17);
            CHECK(memory[activeFwImageAddress0] == 17_b);
            CHECK(memory[activeFwImageAddress1] == 17_b);
            CHECK(memory[activeFwImageAddress2] == 17_b);

            memory[activeFwImageAddress1] = 42_b;
            CHECK(pvs.Load<"activeFwImage">() == 17);
            CHECK(memory[activeFwImageAddress0] == 17_b);
            CHECK(memory[activeFwImageAddress1] == 17_b);
            CHECK(memory[activeFwImageAddress2] == 17_b);

            memory[activeFwImageAddress2] = 42_b;
            CHECK(pvs.Load<"activeFwImage">() == 17);
            CHECK(memory[activeFwImageAddress0] == 17_b);
            CHECK(memory[activeFwImageAddress1] == 17_b);
            CHECK(memory[activeFwImageAddress2] == 17_b);

            memory[activeFwImageAddress0] = 42_b;
            memory[activeFwImageAddress2] = 42_b;
            CHECK(pvs.Load<"activeFwImage">() == 42);
            CHECK(memory[activeFwImageAddress0] == 42_b);
            CHECK(memory[activeFwImageAddress1] == 42_b);
            CHECK(memory[activeFwImageAddress2] == 42_b);
        }
    }

    SECTION("FRAM is not working")
    {
        memory.fill(0x00_b);
        framIsWorking = false;

        SECTION("You load what you store")
        {
            pvs.Store<"nResets">(0xABCDABCD);
            pvs.Store<"activeFwImage">(111);
            pvs.Store<"somethingElse">(-55);
            CHECK(pvs.Load<"nResets">() == 0xABCDABCD);
            CHECK(pvs.Load<"activeFwImage">() == 111);
            CHECK(pvs.Load<"somethingElse">() == -55);
        }

        SECTION("Store() does not write to memory")
        {
            pvs.Store<"nResets">(0xABCDABCD);
            pvs.Store<"activeFwImage">(111);
            pvs.Store<"somethingElse">(-55);
            CHECK(memory[nResetsAddress0] == 0x00_b);
            CHECK(memory[nResetsAddress0 + 1] == 0x00_b);
            CHECK(memory[nResetsAddress0 + 2] == 0x00_b);
            CHECK(memory[nResetsAddress0 + 3] == 0x00_b);
            CHECK(memory[activeFwImageAddress0] == 00_b);
            CHECK(memory[somethingElseAddress0] == 0x00_b);
            CHECK(memory[somethingElseAddress0 + 1] == 0x00_b);
        }

        SECTION("Load() does not read from memory")
        {
            pvs.Store<"activeFwImage">(0);
            memory[activeFwImageAddress0] = 17_b;
            memory[activeFwImageAddress1] = 17_b;
            memory[activeFwImageAddress2] = 17_b;
            CHECK(pvs.Load<"activeFwImage">() == 0);
        }
    }
}
