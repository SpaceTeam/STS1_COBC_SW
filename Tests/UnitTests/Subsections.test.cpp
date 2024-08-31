#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/FramSections/SubsectionInfo.hpp>
#include <Sts1CobcSw/FramSections/Subsections.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Utility/StringLiteral.hpp>

#include <catch2/catch_test_macros.hpp>
#include <strong_type/equality.hpp>
#include <strong_type/type.hpp>


using sts1cobcsw::Section;
using sts1cobcsw::SubsectionInfo;
using sts1cobcsw::Subsections;
using sts1cobcsw::fram::Address;
using sts1cobcsw::fram::Size;


TEST_CASE("All static_asserts passed")
{
    REQUIRE(true);
}


constexpr auto memory = Section<Address(0), Size(1200)>();
constexpr auto sections = Subsections<memory,
                                      SubsectionInfo<"PersistentVariables0", Size(100)>,
                                      SubsectionInfo<"PersistentVariables1", Size(100)>,
                                      SubsectionInfo<"PersistentVariables2", Size(100)>,
                                      SubsectionInfo<"EduProgramQueue", Size(200)>,
                                      SubsectionInfo<"TestMemory", Size(300)>,
                                      SubsectionInfo<"Telemetry", Size(400)> >();

// Does and should not compile: constraints not satisfied [...] because 'sizeof...(SubsectionInfos)
// > 0' (0 > 0) evaluated to false
// constexpr auto sections1 = Subsections<memory>();

// Does and should not compile: static assertion failed [...]: The subsections do not fit into the
// parent section
// constexpr auto sections2 = Subsections<Section<Address(100), Size(200)>{},
//                                        SubsectionInfo<"one", Size(100)>,
//                                        SubsectionInfo<"two", Size(100)>,
//                                        SubsectionInfo<"three", Size(1)> >();


static_assert(sections.Index<"PersistentVariables0">() == 0);
static_assert(sections.Index<"TestMemory">() == 4);
// Does and should not compile: static assertion failed [...]: There is no subsection with this name
// static_assert(sections.Index<"ASDF">() == 2);

constexpr auto telemetry = sections.Get<"Telemetry">();
static_assert(telemetry.begin == Address(800));
static_assert(telemetry.size == Size(400));
static_assert(telemetry.end == Address(1200));
