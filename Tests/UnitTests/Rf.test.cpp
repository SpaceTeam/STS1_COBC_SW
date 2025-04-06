#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Rf/Rf.hpp>


namespace rf = sts1cobcsw::rf;


TEST_CASE("RF module")
{
    rf::Initialize(rf::TxType::morse);
    auto partNumber = rf::ReadPartNumber();
    CHECK(partNumber == rf::correctPartNumber);
}
