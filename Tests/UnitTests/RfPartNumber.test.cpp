#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Rf/Rf.hpp>


namespace rf = sts1cobcsw::rf;


TEST_CASE("Check RF part number")
{
    rf::Initialize(rf::TxType::packet);
    auto partNumber = rf::ReadPartNumber();
    CHECK(partNumber == rf::correctPartNumber);
}
