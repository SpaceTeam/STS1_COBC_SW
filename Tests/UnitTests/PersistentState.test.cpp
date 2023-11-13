// Only the current dummy implementation can be a unit test. The real implementation will be a
// hardware test.

#include <Sts1CobcSw/Periphery/PersistentState.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>


namespace ps = sts1cobcsw::persistentstate;


TEST_CASE("Persistent state getters and setters")
{
    ps::Initialize();

    ps::NotOkCounter(13);
    REQUIRE(ps::NotOkCounter() == 13);

    ps::ActiveFirmwareImage(5);
    REQUIRE(ps::ActiveFirmwareImage() == 5);

    ps::BackupFirmwareImage(9);
    REQUIRE(ps::BackupFirmwareImage() == 9);

    ps::AntennasShouldBeDeployed(/*value=*/false);
    REQUIRE(ps::AntennasShouldBeDeployed() == false);

    ps::TxIsOn(/*value=*/false);
    REQUIRE(ps::TxIsOn() == false);
    ps::TxIsOn(/*value=*/true);
    REQUIRE(ps::TxIsOn() == true);

    ps::EduShouldBePowered(/*value=*/true);
    REQUIRE(ps::EduShouldBePowered() == true);
    ps::EduShouldBePowered(/*value=*/false);
    REQUIRE(ps::EduShouldBePowered() == false);

    ps::UtcOffset(13579);
    REQUIRE(ps::UtcOffset() == 13579);
}
