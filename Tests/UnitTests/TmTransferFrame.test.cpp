#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/ChannelCoding/ChannelCoding.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Payload.hpp>
#include <Sts1CobcSw/RfProtocols/TmTransferFrame.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <etl/vector.h>

#include <array>
#include <cstdint>
#include <span>


// I put this test in the sts1cobcsw namespace because "tm" is a struct defined in the standard
// library so I can't use it as a namespace alias for sts1cobcsw::tm.
namespace sts1cobcsw
{
class TestPayload : public Payload
{
public:
    TestPayload(Byte fillValue, std::uint16_t size) : fillValue_(fillValue), size_(size)
    {}


private:
    Byte fillValue_;
    std::uint16_t size_;

    auto DoAddTo(etl::ivector<Byte> * dataField) const -> void override
    {
        dataField->resize(dataField->size() + size_, fillValue_);
    }


    [[nodiscard]] auto DoSize() const -> std::uint16_t override
    {
        return size_;
    }
};


TEST_CASE("TM Transfer Frame")
{
    auto block = std::array<Byte, blockLength>{};
    auto frame = tm::TransferFrame(Span(&block).first<tm::transferFrameLength>());
    CHECK(block == (std::array<Byte, blockLength>{}));

    frame.StartNew(pusVcid);
    auto & dataField = frame.GetDataField();
    dataField.resize(dataField.max_size(), 0x0F_b);
    frame.Finish();
    auto header = Span(block).first<tm::transferFramePrimaryHeaderLength>();
    CHECK(header[0] == 0b0001'0010_b);  // Always the same
    CHECK(header[1] == 0b0011'0110_b);  // VCID is bits 1-3
    CHECK(header[2] == 0_b);            // Master channel frame count
    CHECK(header[3] == 0_b);            // Virtual channel frame count
    CHECK(header[4] == 0b0001'1000_b);  // Always the same
    CHECK(header[5] == 0b0000'0000_b);  // Always the same
    auto dataFieldSpan =
        Span(block).subspan<tm::transferFramePrimaryHeaderLength, tm::transferFrameDataLength>();
    for(auto byte : dataFieldSpan)
    {
        CHECK(byte == 0x0F_b);
    }
    for(auto byte : Span(block).subspan<tm::transferFrameLength>())
    {
        CHECK(byte == 0x00_b);
    }

    frame.StartNew(pusVcid);
    frame.Finish();
    CHECK(header[1] == 0b0011'0110_b);  // VCID is bits 1-3
    CHECK(header[2] == 1_b);            // Master channel frame count
    CHECK(header[3] == 1_b);            // Virtual channel frame count

    frame.StartNew(pusVcid);
    frame.Finish();
    CHECK(header[1] == 0b0011'0110_b);  // VCID is bits 1-3
    CHECK(header[2] == 2_b);            // Master channel frame count
    CHECK(header[3] == 2_b);            // Virtual channel frame count

    frame.StartNew(cfdpVcid);
    auto addResult =
        frame.Add(TestPayload(0x3C_b, static_cast<std::uint16_t>(dataField.available())));
    CHECK(addResult.has_error() == false);
    frame.Finish();
    CHECK(header[0] == 0b0001'0010_b);  // Always the same
    CHECK(header[1] == 0b0011'1010_b);  // VCID is bits 1-3
    CHECK(header[2] == 3_b);            // Master channel frame count
    CHECK(header[3] == 0_b);            // Virtual channel frame count
    CHECK(header[4] == 0b0001'1000_b);  // Always the same
    CHECK(header[5] == 0b0000'0000_b);  // Always the same
    for(auto byte : dataFieldSpan)
    {
        CHECK(byte == 0x3C_b);
    }
}
}
