#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Rf/Rf.hpp>


namespace rf = sts1cobcsw::rf;


TEST_CASE("RF module")
{
    rf::Initialize(rf::TxType::packet);
    CHECK(rf::ReadPartNumber() == rf::correctPartNumber);
    CHECK(rf::GetDataRate() == 384'000U);

    auto dataRate = 1'200U;
    rf::SetDataRate(dataRate);
    CHECK(rf::GetDataRate() == dataRate);

    dataRate = 9'600U;
    rf::SetDataRate(dataRate);
    CHECK(rf::GetDataRate() == dataRate);
}
