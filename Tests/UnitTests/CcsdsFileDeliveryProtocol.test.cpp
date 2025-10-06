#include <Sts1CobcSw/RfProtocols/CcsdsFileDeliveryProtocol.hpp>
#include <Sts1CobcSw/RfProtocols/ProtocolDataUnits.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <catch2/catch_test_macros.hpp>

#include <etl/utility.h>
#include <etl/vector.h>

#include <array>
#include <span>


// NOLINT(cert-err58-cpp)
using sts1cobcsw::Byte;
using sts1cobcsw::FileDataPdu;
using sts1cobcsw::SegmentRequest;


TEST_CASE("UpdateMissingFileData")
{
    using sts1cobcsw::UpdateMissingFileData;

    auto missingFileData = etl::vector<SegmentRequest, 4>{};
    missingFileData = {
        SegmentRequest{.startOffset = 0, .endOffset = 100}
    };

    UpdateMissingFileData(&missingFileData, FileDataPdu(200, std::array<Byte, 10>{}));
    REQUIRE(missingFileData.size() == 1);
    CHECK(missingFileData[0].startOffset == 0);
    CHECK(missingFileData[0].endOffset == 100);

    UpdateMissingFileData(&missingFileData, FileDataPdu(0, std::array<Byte, 10>{}));
    REQUIRE(missingFileData.size() == 1);
    CHECK(missingFileData[0].startOffset == 10);
    CHECK(missingFileData[0].endOffset == 100);

    UpdateMissingFileData(&missingFileData, FileDataPdu(90, std::array<Byte, 10>{}));
    REQUIRE(missingFileData.size() == 1);
    CHECK(missingFileData[0].startOffset == 10);
    CHECK(missingFileData[0].endOffset == 90);

    UpdateMissingFileData(&missingFileData, FileDataPdu(40, std::array<Byte, 30>{}));
    REQUIRE(missingFileData.size() == 2);
    CHECK(missingFileData[0].startOffset == 10);
    CHECK(missingFileData[0].endOffset == 40);
    CHECK(missingFileData[1].startOffset == 70);
    CHECK(missingFileData[1].endOffset == 90);

    UpdateMissingFileData(&missingFileData, FileDataPdu(50, std::array<Byte, 10>{}));
    REQUIRE(missingFileData.size() == 2);
    CHECK(missingFileData[0].startOffset == 10);
    CHECK(missingFileData[0].endOffset == 40);
    CHECK(missingFileData[1].startOffset == 70);
    CHECK(missingFileData[1].endOffset == 90);

    UpdateMissingFileData(&missingFileData, FileDataPdu(10, std::array<Byte, 30>{}));
    REQUIRE(missingFileData.size() == 1);
    CHECK(missingFileData[0].startOffset == 70);
    CHECK(missingFileData[0].endOffset == 90);

    UpdateMissingFileData(&missingFileData, FileDataPdu(72, std::array<Byte, 2>{}));
    UpdateMissingFileData(&missingFileData, FileDataPdu(76, std::array<Byte, 2>{}));
    UpdateMissingFileData(&missingFileData, FileDataPdu(80, std::array<Byte, 2>{}));
    REQUIRE(missingFileData.size() == 4);

    // If splitting a segment would create more segments than the vector can hold, the missing
    // segments remain unchanged
    auto oldMissingFileData = missingFileData;
    UpdateMissingFileData(&missingFileData, FileDataPdu(84, std::array<Byte, 2>{}));
    REQUIRE(missingFileData.size() == 4);
    for(auto i = 0U; i < missingFileData.size(); ++i)
    {
        CHECK(missingFileData[i].startOffset == oldMissingFileData[i].startOffset);
        CHECK(missingFileData[i].endOffset == oldMissingFileData[i].endOffset);
    }

    // UpdateMissingFileData assumes that the new file data do not span multiple missing segments
    UpdateMissingFileData(&missingFileData, FileDataPdu(70, std::array<Byte, 20>{}));
    REQUIRE(missingFileData.size() == 3);

    UpdateMissingFileData(&missingFileData, FileDataPdu(74, std::array<Byte, 2>{}));
    UpdateMissingFileData(&missingFileData, FileDataPdu(78, std::array<Byte, 2>{}));
    UpdateMissingFileData(&missingFileData, FileDataPdu(82, std::array<Byte, 8>{}));
    REQUIRE(missingFileData.empty());
}
