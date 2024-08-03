#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/Section.hpp>

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


constexpr auto memory = Section<Address(350), Size(1200)>();
static_assert(memory.begin == Address(350));
static_assert(memory.size == Size(1200));
static_assert(decltype(memory)::end == Address(1550));

static_assert(sts1cobcsw::fram::isASection<decltype(memory)>);
static_assert(sts1cobcsw::fram::isASection<Section<Address(10), Size(17)>>);


template<Address sectionBegin, Size sectionSize>
struct NotASection
{
    static constexpr auto begin = sectionBegin;
    static constexpr auto size = sectionSize;
    static constexpr auto end = begin + size;
};


static_assert(not sts1cobcsw::fram::isASection<NotASection<Address(100), Size(348)>>);
