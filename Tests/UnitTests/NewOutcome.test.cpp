// #include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/FileSystem/ErrorsAndResult.hpp>

#include <catch2/catch_test_macros.hpp>

#include <littlefs/lfs.h>


auto DoSomething() -> sts1cobcsw::fs::Result<void>
{
    return outcome_v2::success();
}


TEST_CASE("Always passes")
{
    CHECK(true);
    auto doSomethingResult = DoSomething();
    CHECK(not doSomethingResult.has_error());
}
