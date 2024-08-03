#include <Sts1CobcSw/FramSections/SubsectionInfo.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Utility/StringLiteral.hpp>

#include <catch2/catch_test_macros.hpp>
#include <strong_type/type.hpp>


using sts1cobcsw::containsNoDuplicateNames;
using sts1cobcsw::nameAppearsOnce;
using sts1cobcsw::SubsectionInfo;
using sts1cobcsw::SubsectionInfoLike;
using sts1cobcsw::fram::Size;


TEST_CASE("All static_asserts passed")
{
    REQUIRE(true);
}


constexpr auto ss = SubsectionInfo<"Telemetry", Size(100)>();
static_assert(SubsectionInfoLike<decltype(ss)>);


static_assert(nameAppearsOnce<SubsectionInfo<"a", Size(1)>,  //
                              SubsectionInfo<"a", Size(10)>>);

static_assert(nameAppearsOnce<SubsectionInfo<"a", Size(1)>,
                              SubsectionInfo<"b", Size(1)>,
                              SubsectionInfo<"a", Size(1)>,
                              SubsectionInfo<"c", Size(1)>>);

static_assert(not nameAppearsOnce<SubsectionInfo<"a", Size(1)>,
                                  SubsectionInfo<"x", Size(1)>,
                                  SubsectionInfo<"y", Size(1)>,
                                  SubsectionInfo<"z", Size(1)>>);

static_assert(not nameAppearsOnce<SubsectionInfo<"a", Size(1)>,
                                  SubsectionInfo<"b", Size(1)>,
                                  SubsectionInfo<"a", Size(1)>,
                                  SubsectionInfo<"a", Size(1)>>);


static_assert(containsNoDuplicateNames<SubsectionInfo<"x", Size(1)>>);

static_assert(containsNoDuplicateNames<SubsectionInfo<"x", Size(1)>,
                                       SubsectionInfo<"y", Size(1)>,
                                       SubsectionInfo<"z", Size(1)>>);

static_assert(not containsNoDuplicateNames<SubsectionInfo<"x", Size(1)>,  //
                                           SubsectionInfo<"x", Size(123)>>);

static_assert(not containsNoDuplicateNames<SubsectionInfo<"i", Size(1)>,
                                           SubsectionInfo<"j", Size(123)>,
                                           SubsectionInfo<"k", Size(1)>,
                                           SubsectionInfo<"j", Size(1)>>);
