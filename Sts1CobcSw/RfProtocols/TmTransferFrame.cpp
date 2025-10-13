#include <Sts1CobcSw/RfProtocols/TmTransferFrame.hpp>


namespace sts1cobcsw::tm
{
TransferFrame::TransferFrame(std::span<Byte, transferFrameLength> buffer) : buffer_(buffer)
{}


auto TransferFrame::StartNew(Vcid vcid) -> void
{
    primaryHeader_.vcid = vcid;
    dataField_.clear();  // Resets size to 0
}


auto TransferFrame::GetDataField() -> etl::vector_ext<Byte> &
{
    return dataField_;
}


auto TransferFrame::Add(Payload const & payload) -> Result<void>
{
    return payload.AddTo(&dataField_);
}


auto TransferFrame::Finish() -> void
{
    primaryHeader_.masterChannelFrameCount = masterChannelFrameCounters.PostIncrement(spacecraftId);
    primaryHeader_.virtualChannelFrameCount =
        virtualChannelFrameCounters.PostIncrement(primaryHeader_.vcid);
    (void)SerializeTo<ccsdsEndianness>(buffer_.data(), primaryHeader_);
    // TODO: Fill the remaining space with a proper idle packet
    // FIXME: Is idleData correct?
    dataField_.resize(dataField_.max_size(), idleData);
}


template<std::endian endianness>
auto SerializeTo(void * destination, TransferFrame::PrimaryHeader const & header) -> void *
{
    destination = sts1cobcsw::SerializeTo<endianness>(destination,
                                                      header.versionNumber,
                                                      header.spacecraftId.Value(),
                                                      header.vcid.Value(),
                                                      header.operationalControlFlag);
    destination = sts1cobcsw::SerializeTo<endianness>(destination, header.masterChannelFrameCount);
    destination = sts1cobcsw::SerializeTo<endianness>(destination, header.virtualChannelFrameCount);
    destination = SerializeTo<endianness>(destination,
                                          header.secondaryHeaderFlag,
                                          header.synchronizationFlag,
                                          header.packetOrderFlag,
                                          header.segmentLengthId,
                                          header.firstHeaderPointer);
    return destination;
}


template auto SerializeTo<std::endian::big>(void *, TransferFrame::PrimaryHeader const &) -> void *;
}
