#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/Rf/Rf.hpp>
#include <Sts1CobcSw/Rf/RfDataRateConfigs.hpp>


namespace rf = sts1cobcsw::rf;


TEST_CASE("RF module")
{
    // We need to initialize the FRAM too because the RF code uses persistent variables
    sts1cobcsw::fram::Initialize();
    auto initializeResult = rf::Initialize();
    REQUIRE(initializeResult.has_value());
    auto partNumber = rf::ReadPartNumber();
    CHECK(partNumber == rf::correctPartNumber);

    auto defaultDataRate = sts1cobcsw::rf::dataRateConfig9600;
    CHECK(rf::GetTxDataRate() == defaultDataRate.dataRate);
    CHECK(rf::GetRxDataRate() == defaultDataRate.dataRate);

    auto dataRate = 9600U;
    rf::SetTxDataRate(dataRate);
    rf::SetRxDataRate(dataRate);
    CHECK(rf::GetTxDataRate() == dataRate);
    CHECK(rf::GetRxDataRate() == dataRate);
}
