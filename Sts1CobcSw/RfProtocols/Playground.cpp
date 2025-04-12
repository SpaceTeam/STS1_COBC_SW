// This file contains playground code for testing various implementations of the CCSDS Telemetry
// Transfer Frame Protocol, the CCSDS Space Packet Protocol and the ECSS Packet Utilisation
// Standard.
//
// Assumptions:
// - No segmantation of packets
// - Frames consist of a single ECC block

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/FlatArray.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <etl/vector.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw
{
using sts1cobcsw::operator""_b;


// --- Config ---

inline constexpr std::uint16_t spacecraftId = 1234;     // 10 bit
inline constexpr std::uint8_t pusVcid = 0b011;          // 3 bit
inline constexpr std::uint8_t cfdpVcid = 0b101;         // 3 bit
inline constexpr std::uint16_t apid = 0b000'1100'1100;  // 11 bit

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
inline constexpr auto spacePacketHeaderLength = 4;
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
        std::uint16_t dataLength = 0;
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
        // This is a hack to set the correct data length without having to modify the primaryHeader_
        // in a const member function
        auto serializedHeader = FlatArray(Serialize(primaryHeader.apid),
                                          Serialize(static_cast<std::uint16_t>(dataField.size())));
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
        std::uint16_t spacecraftId = 0;
        std::uint16_t vcid = 0;
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
    auto packet = SpacePacket{.primaryHeader = {.apid = apid}, .dataField = message};
    auto frame = TmTransferFrame<&FillWithIdleData>{
        .primaryHeader = {.spacecraftId = spacecraftId, .vcid = pusVcid},
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
class TmTransferFrame
{
public:
    explicit TmTransferFrame(std::span<Byte, tm::transferFrameLength> buffer) : buffer_(buffer)
    {
    }

    [[nodiscard]] auto GetDataField() -> etl::vector_ext<Byte> &
    {
        return dataField_;
    }

    [[nodiscard]] auto GetDataField() const -> etl::vector_ext<Byte> const &
    {
        return dataField_;
    }

    auto Start(std::uint16_t spacecraftId, std::uint8_t vcid) -> void
    {
        primaryHeader_.spacecraftId = spacecraftId;
        primaryHeader_.vcid = vcid;
        // TODO: Maybe fill with idleData instead?
        std::fill(buffer_.begin(), buffer_.end(), 0x00_b);
        dataField_.clear();  // Reset size to 0
    }

    auto Finish() -> void
    {
        // TODO: Set master channel and virtual channel frame counts
        auto serializedHeader =
            FlatArray(Serialize(primaryHeader_.spacecraftId), Serialize(primaryHeader_.vcid));
        std::copy(serializedHeader.begin(), serializedHeader.end(), buffer_.begin());
        // TODO: Properly fill the remaining space with an idle packet
        dataField_.resize(dataField_.max_size(), idleData);
    }


private:
    struct PrimaryHeader
    {
        std::uint16_t spacecraftId = 0;
        std::uint16_t vcid = 0;
    };
    static_assert(sizeof(PrimaryHeader) == tm::transferFrameHeaderLength);

    PrimaryHeader primaryHeader_;
    std::span<Byte, tm::transferFrameLength> buffer_;
    etl::vector_ext<Byte> dataField_ = etl::vector_ext<Byte>(
        buffer_.data() + tm::transferFrameHeaderLength, tm::transferFrameDataFieldLength);
};


class SpacePacket
{
public:
    explicit SpacePacket(etl::vector_ext<Byte> * buffer) : buffer_(*buffer)
    {
    }

    auto Start() -> void
    {
        packetBegin_ = buffer_.end();
        buffer_.resize(buffer_.size() + spacePacketHeaderLength);
    }

    auto Finish() -> void
    {
        primaryHeader_.dataLength =
            static_cast<std::uint16_t>(buffer_.end() - packetBegin_ - spacePacketHeaderLength);
        auto serializedHeader =
            FlatArray(Serialize(primaryHeader_.apid), Serialize(primaryHeader_.dataLength));
        std::copy(serializedHeader.begin(), serializedHeader.end(), packetBegin_);
    }

    [[nodiscard]] auto GetBuffer() -> etl::vector_ext<Byte> &
    {
        return buffer_;
    }

    [[nodiscard]] auto GetBuffer() const -> etl::vector_ext<Byte> const &
    {
        return buffer_;
    }


private:
    struct PrimaryHeader
    {
        std::uint16_t apid = 0;
        std::uint16_t dataLength = 0;
    };
    static_assert(sizeof(PrimaryHeader) == spacePacketHeaderLength);

    PrimaryHeader primaryHeader_;
    etl::vector_ext<Byte> & buffer_;
    etl::vector_ext<Byte>::iterator packetBegin_ = buffer_.end();
};


auto AppendAck(etl::vector_ext<Byte> * buffer) -> void
{
    buffer->push_back(0x01_b);
}


auto AppendNack(etl::vector_ext<Byte> * buffer, ErrorCode errorCode) -> void
{
    buffer->push_back(0x02_b);
    auto oldSize = buffer->size();
    buffer->resize(buffer->size() + serialSize<ErrorCode>);
    (void)SerializeTo<std::endian::little>(buffer->data() + oldSize, errorCode);
}


auto TestBuffer() -> void
{
    auto eccBlock = ecc::Block{};
    auto frame = TmTransferFrame(Span(&eccBlock).first<tm::transferFrameLength>());
    frame.Start(spacecraftId, pusVcid);
    auto packet = SpacePacket(&frame.GetDataField());
    packet.Start();
    AppendAck(&packet.GetBuffer());
    packet.Finish();
    frame.Finish();
    // auto encodedFrame = Encode(frame);
    // rf::SendAndWait(Span(encodedFrame));
}
}
}
