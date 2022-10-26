// Only the current dummy implementation can be a unit test. The real implementation will be a
// hardware test.

#include <Sts1CobcSw/Periphery/PersistentState.hpp>

#include <catch2/catch_test_macros.hpp>
#include <type_safe/types.hpp>

#include <type_traits>


namespace ps = sts1cobcsw::periphery::persistentstate;


TEST_CASE("Persistent state getters and setters")
{
    using type_safe::operator""_i8;
    using type_safe::operator""_i32;

    ps::Initialize();

    // .get() and == true/false are only used because the Catch2 magic can't handle type_safe
    // numbers

    ps::NotOkCounter(13_i8);
    REQUIRE(ps::NotOkCounter().get() == 13);

    ps::ActiveFirmwareImage(5_i8);
    REQUIRE(ps::ActiveFirmwareImage().get() == 5);

    ps::BackupFirmwareImage(9_i8);
    REQUIRE(ps::BackupFirmwareImage().get() == 9);

    ps::AntennasShouldBeDeployed(false);
    REQUIRE(ps::AntennasShouldBeDeployed() == false);

    ps::TxIsOn(false);
    REQUIRE(ps::TxIsOn() == false);
    ps::TxIsOn(true);
    REQUIRE(ps::TxIsOn() == true);

    ps::EduShouldBePowered(true);
    REQUIRE(ps::EduShouldBePowered() == true);
    ps::EduShouldBePowered(false);
    REQUIRE(ps::EduShouldBePowered() == false);

    ps::UtcOffset(13579);
    REQUIRE(ps::UtcOffset().get() == 13579);
}
