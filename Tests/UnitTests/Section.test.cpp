#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>

#include <catch2/catch_test_macros.hpp>
#include <strong_type/affine_point.hpp>
#include <strong_type/equality.hpp>
#include <strong_type/type.hpp>


using sts1cobcsw::fram::Address;
using sts1cobcsw::fram::Section;
using sts1cobcsw::fram::Size;


TEST_CASE("All static_asserts passed")
{
    REQUIRE(true);
}


constexpr auto section = Section<Address(350), Size(1200)>();
static_assert(section.begin == Address(350));
static_assert(section.size == Size(1200));
static_assert(decltype(section)::end == Address(1550));
