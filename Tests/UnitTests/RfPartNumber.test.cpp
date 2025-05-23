#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Rf/Rf.hpp>


namespace rf = sts1cobcsw::rf;


TEST_CASE("RF module")
{
    rf::Initialize(rf::TxType::packet);
    auto partNumber = rf::ReadPartNumber();
    CHECK(partNumber == rf::correctPartNumber);

    static constexpr auto baudrate = 6'000'000U;
    rf::SetBaudrate(baudrate);
    CHECK(rf::GetBaudrate() == baudrate);
}
