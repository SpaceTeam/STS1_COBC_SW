#include <Sts1CobcSw/RfProtocols/TcTransferFrame.hpp>

#include <Sts1CobcSw/Blake2s/Blake2s.hpp>

#include <array>


namespace sts1cobcsw::tc
{
auto ParseAsTransferFrame(std::span<Byte const, transferFrameLength> buffer)
    -> Result<TransferFrame>
{
    auto frame = TransferFrame{
        .dataField = buffer.subspan<transferFramePrimaryHeaderLength + securityHeaderLength,
                                    transferFrameDataLength>()};
    auto const * cursor = DeserializeFrom<ccsdsEndianness>(buffer.data(), &frame.primaryHeader);
    auto frameIsValid = frame.primaryHeader.versionNumber == transferFrameVersionNumber
                    and frame.primaryHeader.bypassFlag == 1
                    and frame.primaryHeader.controlCommandFlag == 0;
    if(not frameIsValid)
    {
        return ErrorCode::invalidTransferFrame;
    }
    if(not IsValid(frame.primaryHeader.spacecraftId))
    {
        return ErrorCode::invalidSpacecraftId;
    }
    if(not(frame.primaryHeader.extraVcidBits == 0 and IsValid(frame.primaryHeader.vcid)))
    {
        return ErrorCode::invalidVcid;
    }
    if(frame.primaryHeader.frameLength != transferFrameLength - 1)
    {
        return ErrorCode::invalidFrameLength;
    }
    (void)sts1cobcsw::DeserializeFrom<ccsdsEndianness>(cursor, &frame.securityParameterIndex);
    if(frame.securityParameterIndex != securityParameterIndex)
    {
        return ErrorCode::invalidSecurityParameterIndex;
    }
    (void)sts1cobcsw::DeserializeFrom<ccsdsEndianness>(buffer.last<securityTrailerLength>().data(),
                                                       &frame.messageAuthenticationCode);
    auto messageAuthenticationCode =
        blake2s::ComputeHash(buffer.first<transferFrameLength - securityTrailerLength>());
    if(frame.messageAuthenticationCode != messageAuthenticationCode)
    {
        return ErrorCode::authenticationFailed;
    }
    return frame;
}


template<std::endian endianness>
auto DeserializeFrom(void const * source, TransferFrame::PrimaryHeader * header) -> void const *
{
    auto spacecraftIdValue = SpacecraftId::ValueType{};
    auto vcidValue = Vcid::ValueType{};
    source = sts1cobcsw::DeserializeFrom<endianness>(source,
                                                     &header->versionNumber,
                                                     &header->bypassFlag,
                                                     &header->controlCommandFlag,
                                                     &header->spare,
                                                     &spacecraftIdValue,
                                                     &header->extraVcidBits,
                                                     &vcidValue,
                                                     &header->frameLength);
    header->spacecraftId = SpacecraftId(spacecraftIdValue);
    header->vcid = Vcid(vcidValue);
    source = sts1cobcsw::DeserializeFrom<endianness>(source, &header->frameSequenceNumber);
    return source;
}


template auto DeserializeFrom<std::endian::big>(void const * source,
                                                TransferFrame::PrimaryHeader * header)
    -> void const *;
}
