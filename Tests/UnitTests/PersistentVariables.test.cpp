#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/Fram/FramMock.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariableInfo.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/Utility/StringLiteral.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <array>
#include <cstdint>
#include <type_traits>
#include <utility>


using sts1cobcsw::PersistentVariableInfo;
using sts1cobcsw::PersistentVariables;
using sts1cobcsw::Section;
using sts1cobcsw::fram::Address;
using sts1cobcsw::fram::Size;


constexpr auto pvs = PersistentVariables<Section<Address(0), Size(60)>{},
                                         PersistentVariableInfo<"nResets", std::uint32_t>,
                                         PersistentVariableInfo<"activeFwImage", std::int8_t>,
                                         PersistentVariableInfo<"somethingElse", std::int16_t>>{};

static_assert(std::is_same_v<decltype(pvs)::ValueType<"nResets">, std::uint32_t>);
static_assert(std::is_same_v<decltype(pvs)::ValueType<"activeFwImage">, std::int8_t>);
static_assert(std::is_same_v<decltype(pvs)::ValueType<"somethingElse">, std::int16_t>);

static_assert(std::is_same_v<decltype(pvs.Load<"nResets">()), std::uint32_t>);
static_assert(std::is_same_v<decltype(pvs.Load<"activeFwImage">()), std::int8_t>);
static_assert(std::is_same_v<decltype(pvs.Load<"somethingElse">()), std::int16_t>);


TEST_CASE("PersistentVariables")
{
    // TEST_CASE("Load() and Store()")
    using sts1cobcsw::fram::Address;
    using sts1cobcsw::fram::framIsWorking;
    using sts1cobcsw::fram::ram::memory;
    using sts1cobcsw::operator""_b;

    constexpr auto nResetsAddress0 = 0;
    constexpr auto activeFwImageAddress0 = 4;
    constexpr auto somethingElseAddress0 = 5;
    constexpr auto activeFwImageAddress1 = activeFwImageAddress0 + value_of(pvs.section.size / 3);
    constexpr auto activeFwImageAddress2 = activeFwImageAddress1 + value_of(pvs.section.size / 3);

    sts1cobcsw::fram::ram::SetAllDoFunctions();

    // SECTION("FRAM is working")
    {
        memory.fill(0x00_b);
        framIsWorking.Store(true);

        // SECTION("You load what you store")
        {
            pvs.Store<"nResets">(0x1234'5678);
            pvs.Store<"activeFwImage">(42);
            pvs.Store<"somethingElse">(-2);
            CHECK(pvs.Load<"nResets">() == 0x1234'5678U);
            CHECK(pvs.Load<"activeFwImage">() == 42);
            CHECK(pvs.Load<"somethingElse">() == -2);
        }

        // SECTION("Increment() increments")
        {
            pvs.Store<"nResets">(13U);
            pvs.Increment<"nResets">();
            CHECK(pvs.Load<"nResets">() == 14U);
            pvs.Increment<"nResets">();
            CHECK(pvs.Load<"nResets">() == 15U);
        }

        // SECTION("Add() adds")
        {
            pvs.Store<"nResets">(13U);
            pvs.Add<"nResets">(7U);
            CHECK(pvs.Load<"nResets">() == 20U);
            pvs.Add<"nResets">(-3U);
            CHECK(pvs.Load<"nResets">() == 17U);
        }

        // SECTION("Store() writes to memory")
        {
            pvs.Store<"nResets">(0x1234'5678U);
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

        // SECTION("Load() reads from memory")
        {
            memory[activeFwImageAddress0] = 17_b;
            memory[activeFwImageAddress1] = 17_b;
            memory[activeFwImageAddress2] = 17_b;
            CHECK(pvs.Load<"activeFwImage">() == 17);
        }

        // SECTION("Increment() loads from and writes to memory")
        {
            memory[activeFwImageAddress0] = 17_b;
            memory[activeFwImageAddress1] = 17_b;
            memory[activeFwImageAddress2] = 17_b;
            pvs.Increment<"activeFwImage">();
            CHECK(memory[activeFwImageAddress0] == 18_b);
            CHECK(memory[activeFwImageAddress1] == 18_b);
            CHECK(memory[activeFwImageAddress2] == 18_b);
        }

        // SECTION("Add() loads from and writes to memory")
        {
            memory[activeFwImageAddress0] = 17_b;
            memory[activeFwImageAddress1] = 17_b;
            memory[activeFwImageAddress2] = 17_b;
            pvs.Add<"activeFwImage">(5);
            CHECK(memory[activeFwImageAddress0] == 22_b);
            CHECK(memory[activeFwImageAddress1] == 22_b);
            CHECK(memory[activeFwImageAddress2] == 22_b);
        }

        // SECTION("Load() returns majority and repairs memory")
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

        // SECTION("Increment() increments majority and repairs memory")
        {
            memory[activeFwImageAddress0] = 42_b;
            memory[activeFwImageAddress1] = 10_b;
            memory[activeFwImageAddress2] = 10_b;
            pvs.Increment<"activeFwImage">();
            CHECK(pvs.Load<"activeFwImage">() == 11);
            CHECK(memory[activeFwImageAddress0] == 11_b);
            CHECK(memory[activeFwImageAddress1] == 11_b);
            CHECK(memory[activeFwImageAddress2] == 11_b);

            memory[activeFwImageAddress1] = 42_b;
            pvs.Increment<"activeFwImage">();
            CHECK(pvs.Load<"activeFwImage">() == 12);
            CHECK(memory[activeFwImageAddress0] == 12_b);
            CHECK(memory[activeFwImageAddress1] == 12_b);
            CHECK(memory[activeFwImageAddress2] == 12_b);

            memory[activeFwImageAddress2] = 42_b;
            pvs.Increment<"activeFwImage">();
            CHECK(pvs.Load<"activeFwImage">() == 13);
            CHECK(memory[activeFwImageAddress0] == 13_b);
            CHECK(memory[activeFwImageAddress1] == 13_b);
            CHECK(memory[activeFwImageAddress2] == 13_b);

            memory[activeFwImageAddress0] = 42_b;
            memory[activeFwImageAddress2] = 42_b;
            pvs.Increment<"activeFwImage">();
            CHECK(pvs.Load<"activeFwImage">() == 43);
            CHECK(memory[activeFwImageAddress0] == 43_b);
            CHECK(memory[activeFwImageAddress1] == 43_b);
            CHECK(memory[activeFwImageAddress2] == 43_b);
        }

        // SECTION("Add() adds to majority and repairs memory")
        {
            memory[activeFwImageAddress0] = 42_b;
            memory[activeFwImageAddress1] = 10_b;
            memory[activeFwImageAddress2] = 10_b;
            pvs.Add<"activeFwImage">(5);
            CHECK(pvs.Load<"activeFwImage">() == 15);
            CHECK(memory[activeFwImageAddress0] == 15_b);
            CHECK(memory[activeFwImageAddress1] == 15_b);
            CHECK(memory[activeFwImageAddress2] == 15_b);

            memory[activeFwImageAddress1] = 42_b;
            pvs.Add<"activeFwImage">(5);
            CHECK(pvs.Load<"activeFwImage">() == 20);
            CHECK(memory[activeFwImageAddress0] == 20_b);
            CHECK(memory[activeFwImageAddress1] == 20_b);
            CHECK(memory[activeFwImageAddress2] == 20_b);

            memory[activeFwImageAddress2] = 42_b;
            pvs.Add<"activeFwImage">(5);
            CHECK(pvs.Load<"activeFwImage">() == 25);
            CHECK(memory[activeFwImageAddress0] == 25_b);
            CHECK(memory[activeFwImageAddress1] == 25_b);
            CHECK(memory[activeFwImageAddress2] == 25_b);

            memory[activeFwImageAddress0] = 42_b;
            memory[activeFwImageAddress2] = 42_b;
            pvs.Add<"activeFwImage">(5);
            CHECK(pvs.Load<"activeFwImage">() == 47);
            CHECK(memory[activeFwImageAddress0] == 47_b);
            CHECK(memory[activeFwImageAddress1] == 47_b);
            CHECK(memory[activeFwImageAddress2] == 47_b);
        }
    }

    // SECTION("FRAM is not working")
    {
        memory.fill(0x00_b);
        framIsWorking.Store(false);

        // SECTION("You load what you store")
        {
            pvs.Store<"nResets">(0xABCD'ABCD);
            pvs.Store<"activeFwImage">(111);
            pvs.Store<"somethingElse">(-55);
            CHECK(pvs.Load<"nResets">() == 0xABCD'ABCDU);
            CHECK(pvs.Load<"activeFwImage">() == 111);
            CHECK(pvs.Load<"somethingElse">() == -55);
        }

        // SECTION("Store() does not write to memory")
        {
            pvs.Store<"nResets">(0xABCD'ABCD);
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

        // SECTION("Load() does not read from and write to memory")
        {
            pvs.Store<"activeFwImage">(0);
            memory[activeFwImageAddress0] = 17_b;
            memory[activeFwImageAddress1] = 17_b;
            memory[activeFwImageAddress2] = 17_b;
            CHECK(pvs.Load<"activeFwImage">() == 0);
            CHECK(memory[activeFwImageAddress0] == 17_b);
            CHECK(memory[activeFwImageAddress1] == 17_b);
            CHECK(memory[activeFwImageAddress2] == 17_b);
        }

        // SECTION("Increment() does not load from or write to memory")
        {
            pvs.Store<"activeFwImage">(0);
            memory[activeFwImageAddress0] = 17_b;
            memory[activeFwImageAddress1] = 17_b;
            memory[activeFwImageAddress2] = 17_b;
            pvs.Increment<"activeFwImage">();
            CHECK(pvs.Load<"activeFwImage">() == 1);
            CHECK(memory[activeFwImageAddress0] == 17_b);
            CHECK(memory[activeFwImageAddress1] == 17_b);
            CHECK(memory[activeFwImageAddress2] == 17_b);
        }

        // SECTION("Add() does not load from or write to memory")
        {
            pvs.Store<"activeFwImage">(0);
            memory[activeFwImageAddress0] = 17_b;
            memory[activeFwImageAddress1] = 17_b;
            memory[activeFwImageAddress2] = 17_b;
            pvs.Add<"activeFwImage">(5);
            CHECK(pvs.Load<"activeFwImage">() == 5);
            CHECK(memory[activeFwImageAddress0] == 17_b);
            CHECK(memory[activeFwImageAddress1] == 17_b);
            CHECK(memory[activeFwImageAddress2] == 17_b);
        }
    }
}
