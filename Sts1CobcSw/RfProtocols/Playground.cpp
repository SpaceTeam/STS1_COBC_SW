// This file contains playground code for testing various implementations of the CCSDS Telemetry
// Transfer Frame Protocol, the CCSDS Space Packet Protocol and the ECSS Packet Utilisation
// Standard.
//
// Assumptions:
// - No segmantation of packets
// - Frames consist of a single ECC block

#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/FlatArray.hpp>

#include <etl/vector.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw
{
using sts1cobcsw::operator""_b;

// Config
namespace ecc
{
// RS(255, 223)
inline constexpr auto blockLength = 255;
inline constexpr auto messageLength = 223;
inline constexpr auto nParitySymbols = 32;
static_assert(nParitySymbols == blockLength - messageLength);

using Block = std::array<Byte, blockLength>;
}

namespace tm
{
inline constexpr auto transferFrameLength = ecc::messageLength;
inline constexpr auto transferFrameHeaderLength = 4;
inline constexpr auto transferFrameDataFieldLength =
    transferFrameLength - transferFrameHeaderLength;
}

inline constexpr auto maxSpacePacketLength = tm::transferFrameDataFieldLength;
inline constexpr auto spacePacketHeaderLength = 2;
inline constexpr auto maxSpacePacketDataFieldLength =
    maxSpacePacketLength - spacePacketHeaderLength;

inline constexpr std::uint16_t idlePacketApid = 0x7FFF;
inline constexpr auto idleData = 0x55_b;


// Message is just an etl::vector, Packet and Frame own their data -> The data of the message lies
// in memory a total of 4 times: in the message, packet, frame, and serialized frame
namespace owning
{
struct SpacePacket
{
    struct PrimaryHeader
    {
        std::uint16_t apid;
    };
    static_assert(sizeof(PrimaryHeader) == spacePacketHeaderLength);
    using DataField = etl::vector<Byte, maxSpacePacketDataFieldLength>;

    PrimaryHeader primaryHeader;
    DataField dataField;

    [[nodiscard]] auto Size() const -> std::size_t
    {
        return sizeof(primaryHeader) + dataField.size();
    }

    template<std::size_t size>
    auto SerializeTo(etl::vector<Byte, size> * buffer) const -> void
    {
        assert(buffer->available() >= Size());
        auto serializedHeader = Serialize(primaryHeader.apid);
        buffer->insert(buffer->end(), serializedHeader.begin(), serializedHeader.end());
        buffer->insert(buffer->end(), dataField.begin(), dataField.end());
    }
};

auto Ack() -> SpacePacket::DataField
{
    return {0x01_b};
}


template<void (*idlePacketFiller)(etl::vector<Byte, tm::transferFrameDataFieldLength> & dataField)>
struct TmTransferFrame
{
    struct PrimaryHeader
    {
        std::uint16_t spacecraftId;
        std::uint16_t vcid;
    };
    static_assert(sizeof(PrimaryHeader) == tm::transferFrameHeaderLength);
    using DataField = etl::vector<Byte, tm::transferFrameDataFieldLength>;

    PrimaryHeader primaryHeader;
    DataField dataField;

    template<typename Packet>
    [[nodiscard]] auto TryToAdd(Packet const & packet) -> bool
    {
        if(dataField.available() >= packet.Size())
        {
            packet.SerializeTo(&dataField);
            return true;
        }
        return false;
    }

    auto Finish() -> void
    {
        idlePacketFiller(dataField);
        // Set master channel and virtual channel frame counts
    }

    auto SerializeTo(std::span<Byte> buffer) const -> void
    {
        assert(buffer.size() >= tm::transferFrameLength);
        auto serializedHeader =
            FlatArray(Serialize(primaryHeader.spacecraftId), Serialize(primaryHeader.vcid));
        std::copy(serializedHeader.begin(), serializedHeader.end(), buffer.begin());
        std::copy(dataField.begin(), dataField.end(), buffer.begin() + serializedHeader.size());
    }
};

auto FillWithIdleData(etl::vector<Byte, tm::transferFrameDataFieldLength> & dataField)
{
    dataField.insert(dataField.end(), dataField.available(), idleData);
}


auto TestOwning() -> void
{
    auto message = Ack();
    // SpacePacket could be a function that returns an etl::vector<Byte, maxSpacePacketSize>
    auto packet = SpacePacket{.primaryHeader = {.apid = 3}, .dataField = message};
    auto frame = TmTransferFrame<&FillWithIdleData>{
        .primaryHeader = {.spacecraftId = 1, .vcid = 1},
        .dataField = {},
    };
    (void)frame.TryToAdd(packet);
    frame.Finish();
    // auto encodedFrame = Encode(frame);
    // rf::SendAndWait(Span(encodedFrame));
}
}


// Message is just an etl::vector, Packet and Frame takes references -> The data of the message lies
// in memory a total of 2 times: in the message and serialized frame
namespace references
{
}


// Message, Packet and Frame all get views of a single global buffer into which they directly
// serialize their data -> The data of the message lies in memory only once in that buffer.
namespace buffer
{
auto eccBlock = ecc::Block{};

}
}
