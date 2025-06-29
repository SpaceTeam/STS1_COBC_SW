#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/Rf/Rf.hpp>


namespace rf = sts1cobcsw::rf;


TEST_CASE("Check RF part number")
{
    // We need to initialize the FRAM too because the RF code uses persistent variables
    sts1cobcsw::fram::Initialize();
    auto initializeResult = rf::Initialize(rf::TxType::packet);
    REQUIRE(initializeResult.has_value());
    auto partNumber = rf::ReadPartNumber();
    CHECK(partNumber == rf::correctPartNumber);
}
