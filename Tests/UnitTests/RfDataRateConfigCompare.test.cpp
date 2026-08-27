#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/Rf/Rf.hpp>
#include <Sts1CobcSw/Rf/RfDataRateConfigs.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace rf = sts1cobcsw::rf;
using sts1cobcsw::operator""_b;



TEST_CASE("Rf Data Rate Config Compare")
{
    // We need to initialize the FRAM too because the RF code uses persistent variables
    sts1cobcsw::fram::Initialize();
    auto initializeResult = rf::Initialize();
    REQUIRE(initializeResult.has_value());



    auto defaultDataRate = sts1cobcsw::rf::dataRateConfig9600;
    CHECK(rf::GetTxDataRate() == defaultDataRate.dataRate);
    CHECK(rf::GetRxDataRate() == defaultDataRate.dataRate);

    auto dataRate = 9600U;
    rf::SetTxDataRate(dataRate);
    rf::SetRxDataRate(dataRate);
    CHECK(rf::GetTxDataRate() == dataRate);
    CHECK(rf::GetRxDataRate() == dataRate);


    constexpr auto propertyGroup = rf::PropertyGroup::modem;
    constexpr auto startIndex = 0x00_b;
    constexpr std::size_t nBytes = 12;

    RODOS::PRINTF("--start printf of read bytes--\n");
    auto propertysResult = rf::ReadPropertys(propertyGroup, startIndex, nBytes);
    CHECK(propertysResult.has_value());
    if(propertysResult.has_value())
    {
        auto const & propertyValues = propertysResult.value();
        for(auto value : propertyValues)
        {
            RODOS::PRINTF("0x%02x, ", static_cast<std::uint8_t>(value));
        }
        RODOS::PRINTF("\n");
    }
}
