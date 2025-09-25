#include <Sts1CobcSw/Firmware/FileTransferThread.hpp>

#include <Sts1CobcSw/ChannelCoding/ChannelCoding.hpp>
#include <Sts1CobcSw/ErrorDetectionAndCorrection/EdacVariable.hpp>
#include <Sts1CobcSw/FileSystem/File.hpp>
#include <Sts1CobcSw/Firmware/RfCommunicationThread.hpp>
#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Mailbox/Mailbox.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/Payload.hpp>
#include <Sts1CobcSw/RfProtocols/ProtocolDataUnitHeader.hpp>
#include <Sts1CobcSw/RfProtocols/ProtocolDataUnits.hpp>
#include <Sts1CobcSw/RfProtocols/TmTransferFrame.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/FileTransfer.hpp>
#include <Sts1CobcSw/Vocabulary/Ids.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <littlefs/lfs.h>
#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/equality.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <algorithm>
#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>


namespace sts1cobcsw
{
namespace
{
enum class CancelCondition : std::uint8_t
{
    receivedFinishedPdu,
    receivedEofCancelPdu,
};


enum class InterruptCondition : std::uint8_t
{
    never,
    receivedNakPdu,
    receivedEofNoErrorPdu,
};


constexpr auto stackSize = 2000U;
// This should be more than enough time to actually send the CFDP frame
constexpr auto fileTransferWindowEndMargin = 5 * s;

auto encodedFrame = std::array<Byte, blockLength>{};
auto frame = tm::TransferFrame(std::span(encodedFrame).first<tm::transferFrameLength>());

// Level -1 private functions
auto SendFile(FileTransferMetadata const & fileTransferMetadata) -> void;
auto ReceiveFile(FileTransferMetadata const & fileTransferMetadata) -> void;
auto ReceiveFirmware(FileTransferMetadata const & fileTransferMetadata) -> void;

// Level -2 private functions
template<typename Pdu>
    requires std::is_same_v<Pdu, FileDataPdu> or requires { Pdu::directiveCode; }
auto PackageAndEncode(Pdu const & pdu, EntityId sourceEntityId) -> void;
auto SuspendUntilFrameCanBePublished(CancelCondition cancelCondition,
                                     InterruptCondition interruptCondition) -> Result<void>;
auto Send(fs::File const & file, std::uint32_t fileSize) -> Result<void>;
auto SendAndWaitForAck(EndOfFilePdu const & endOfFilePdu) -> Result<void>;
auto SendAndWaitForAck(FinishedPdu const & finishedPdu) -> Result<void>;

// Level -3 private functions
auto PackageAndEncode(Payload const & pdu, PduType pduType, EntityId sourceEntityId) -> void;
auto Send(fs::File const & file,
          std::uint32_t fileSize,
          std::span<SegmentRequest const> segments,
          InterruptCondition interruptCondition) -> Result<void>;
auto SendAndWaitForAck(Payload const & pdu,
                       DirectiveCode directiveCode,
                       ConditionCode conditionCode,
                       CancelCondition cancelCondition) -> Result<void>;


class FileTransferThread : public RODOS::StaticThread<stackSize>
{
public:
    FileTransferThread() : StaticThread("FileTransferThread", fileTransferThreadPriority)
    {}


private:
    auto run() -> void override
    {
        SuspendFor(totalStartupTestTimeout);  // Wait for the startup tests to complete
        DEBUG_PRINT("Starting file transfer thread\n");
        while(true)
        {
            // There cannot be a timeout error if we wait until the end of time
            (void)fileTransferMetadataMailbox.SuspendUntilFullOr(endOfTime);
            if(fileTransferMetadataMailbox.IsEmpty())
            {
                continue;
            }
            auto fileTransferMetadata = fileTransferMetadataMailbox.Get().value();
            if(fileTransferMetadata.sourceEntityId == cubeSatEntityId)
            {
                DEBUG_PRINT("Sending file to ground station: '%s' -> '%s'\n",
                            fileTransferMetadata.sourcePath.c_str(),
                            fileTransferMetadata.destinationPath.c_str());
                persistentVariables.Increment<"transactionSequenceNumber">();
                transactionSequenceNumber.Store(
                    persistentVariables.Load<"transactionSequenceNumber">());
                fileTransferStatus.Store(FileTransferStatus::sending);
                SendFile(fileTransferMetadata);
            }
            else
            {
                fileTransferStatus.Store(FileTransferStatus::receiving);
                transactionSequenceNumber.Store(unknownTransactionSequenceNumber);
                if(fileTransferMetadata.fileIsFirmware)
                {
                    DEBUG_PRINT("Receiving firmware from ground station: '%s' -> FW partition %s\n",
                                fileTransferMetadata.sourcePath.c_str(),
                                ToCZString(fileTransferMetadata.destinationPartitionId));
                    ReceiveFirmware(fileTransferMetadata);
                }
                else
                {
                    DEBUG_PRINT("Receiving file from ground station: '%s' -> '%s'\n",
                                fileTransferMetadata.sourcePath.c_str(),
                                fileTransferMetadata.destinationPath.c_str());
                    ReceiveFile(fileTransferMetadata);
                }
            }
        }
    }
} fileTransferThread;
}


auto ResumeFileTransferThread() -> void
{
    fileTransferThread.resume();
}


namespace
{
auto SendFile(FileTransferMetadata const & fileTransferMetadata) -> void
{
    // TODO: Think about proper error handling (e.g. file system errors)
    auto result = [&]() -> Result<void>
    {
        OUTCOME_TRY(auto file, fs::Open(fileTransferMetadata.sourcePath, LFS_O_RDONLY));
        OUTCOME_TRY(auto fileSize, file.Size());

        // Publish Metadata PDU
        PackageAndEncode(
            MetadataPdu(
                fileSize, fileTransferMetadata.sourcePath, fileTransferMetadata.destinationPath),
            cubeSatEntityId);
        OUTCOME_TRY(SuspendUntilFrameCanBePublished(CancelCondition::receivedFinishedPdu,
                                                    InterruptCondition::never));
        DEBUG_PRINT("Publishing encoded frame with Metadata PDU\n");
        encodedCfdpFrameMailbox.Overwrite(encodedFrame);
        ResumeRfCommunicationThread();

        OUTCOME_TRY(Send(file, fileSize));
        OUTCOME_TRY(SendAndWaitForAck(EndOfFilePdu(fileSize)));

        // TODO: Continue here!
        // WaitForFinishedOrNakPdusAndResendDataIfNecessary
        ResumeRfCommunicationThread();

        return outcome_v2::success();
    }();
    if(result.has_value())
    {
        DEBUG_PRINT("File sent successfully\n");
    }
    else if(result.error() == ErrorCode::fileTransferCanceled)
    {
        DEBUG_PRINT("Acknowledging cancel request by ground station\n");
        // TODO: Acknowledge cancel request (= send ACK(EOF/FIN) PDU)
    }
    else if(result.error() == ErrorCode::positiveAckLimitReached)
    {
        DEBUG_PRINT("Error sending file: positive ACK limit reached -> abondoning transfer\n");
    }
    else
    {
        DEBUG_PRINT("Error sending file: %s\n", ToCZString(result.error()));
    }
}


auto ReceiveFile(FileTransferMetadata const & fileTransferMetadata) -> void
{
    (void)fileTransferMetadata;
}


auto ReceiveFirmware(FileTransferMetadata const & fileTransferMetadata) -> void
{
    (void)fileTransferMetadata;
}


template<typename Pdu>
    requires std::is_same_v<Pdu, FileDataPdu> or requires { Pdu::directiveCode; }
auto PackageAndEncode(Pdu const & pdu, EntityId sourceEntityId) -> void
{
    if constexpr(std::is_same_v<Pdu, FileDataPdu>)
    {
        PackageAndEncode(pdu, fileDataPduType, sourceEntityId);
    }
    else
    {
        PackageAndEncode(pdu, fileDirectivePduType, sourceEntityId);
    }
}


// TODO: Refactor
auto SuspendUntilFrameCanBePublished(CancelCondition cancelCondition,
                                     InterruptCondition interruptCondition) -> Result<void>
{
    // TODO: We do not check the received PDUs if the encodedCfdpFrameMailbox is not full
    while(encodedCfdpFrameMailbox.IsFull())
    {
        // TODO: Think about the correct reactivation time for all suspend functions
        (void)encodedCfdpFrameMailbox.SuspendUntilEmptyOr(endOfTime);

        // GetReceivedFileDirectivePdu() -> Result<FileDirectivePdu>
        if(receivedPduMailbox.IsEmpty())
        {
            continue;
        }
        auto receivedPdu = receivedPduMailbox.Get().value();
        if(receivedPdu.header.pduType != fileDirectivePduType)
        {
            continue;
        }
        auto parseAsFileDirectiveResult = ParseAsFileDirectivePdu(receivedPdu.dataField);
        if(parseAsFileDirectiveResult.has_error())
        {
            continue;
        }
        auto & fileDirectivePdu = parseAsFileDirectiveResult.value();

        // Check(fileDirectivePdu, cancelCondition, interruptCondition) -> Result<void>
        if(cancelCondition == CancelCondition::receivedFinishedPdu
           and fileDirectivePdu.directiveCode == DirectiveCode::finished)
        {
            return ErrorCode::fileTransferCanceled;
        }
        if((cancelCondition == CancelCondition::receivedEofCancelPdu
            or interruptCondition == InterruptCondition::receivedEofNoErrorPdu)
           and fileDirectivePdu.directiveCode == DirectiveCode::endOfFile)
        {
            auto parseAsEndOfFileResult = ParseAsEndOfFilePdu(fileDirectivePdu.parameterField);
            if(parseAsEndOfFileResult.has_error())
            {
                continue;
            }
            auto & endOfFilePdu = parseAsEndOfFileResult.value();
            if(interruptCondition == InterruptCondition::receivedEofNoErrorPdu
               and endOfFilePdu.conditionCode_ == noErrorConditionCode)
            {
                return ErrorCode::fileTransferInterrupted;
            }
            if(cancelCondition == CancelCondition::receivedEofCancelPdu
               and endOfFilePdu.conditionCode_ != unsupportedChecksumTypeConditionCode)
            {
                return ErrorCode::fileTransferCanceled;
            }
        }
        if(interruptCondition == InterruptCondition::receivedNakPdu
           and fileDirectivePdu.directiveCode == DirectiveCode::nak)
        {
            return ErrorCode::fileTransferInterrupted;
        }
    }
    while(CurrentRodosTime()
          > persistentVariables.Load<"fileTransferWindowEnd">() + fileTransferWindowEndMargin)
    {
        SuspendUntil(endOfTime);
    }
    return outcome_v2::success();
}


auto Send(fs::File const & file, std::uint32_t fileSize) -> Result<void>
{
    return Send(file,
                fileSize,
                Span(SegmentRequest{.startOffset = 0, .endOffset = fileSize}),
                InterruptCondition::never);
}


auto SendAndWaitForAck(EndOfFilePdu const & endOfFilePdu) -> Result<void>
{
    DEBUG_PRINT("Starting pos. acknowledgment procedure for End-of-File PDU\n");
    return SendAndWaitForAck(endOfFilePdu,
                             endOfFilePdu.directiveCode,
                             endOfFilePdu.conditionCode_,
                             CancelCondition::receivedFinishedPdu);
}


auto SendAndWaitForAck(FinishedPdu const & finishedPdu) -> Result<void>
{
    DEBUG_PRINT("Starting pos. acknowledgment procedure for Finished PDU\n");
    return SendAndWaitForAck(finishedPdu,
                             finishedPdu.directiveCode,
                             finishedPdu.conditionCode_,
                             CancelCondition::receivedEofCancelPdu);
}


auto PackageAndEncode(Payload const & pdu, PduType pduType, EntityId sourceEntityId) -> void
{
    frame.StartNew(cfdpVcid);
    // We know that we only get PDUs here and that they have a valid size so AddPduTo() never fails
    (void)AddPduTo(&frame.GetDataField(),
                   pduType,
                   sourceEntityId,
                   persistentVariables.Load<"transactionSequenceNumber">(),
                   pdu);
    frame.Finish();
    tm::Encode(encodedFrame);
}


auto Send(fs::File const & file,
          std::uint32_t fileSize,
          std::span<SegmentRequest const> segments,
          InterruptCondition interruptCondition) -> Result<void>
{
    auto buffer = std::array<Byte, maxFileSegmentLength>{};
    for(auto && segment : segments)
    {
        auto endOffset = std::min(segment.endOffset, fileSize);
        auto startOffset = std::min(segment.startOffset, endOffset);
        OUTCOME_TRY(file.SeekAbsolute(static_cast<int>(startOffset)));
        for(auto i = startOffset; i < endOffset;)
        {
            auto fileData =
                std::span(buffer).first(std::min<std::size_t>(buffer.size(), fileSize - i));
            OUTCOME_TRY(auto nBytesRead, file.Read(fileData));
            fileData = fileData.first(static_cast<unsigned>(nBytesRead));
            i += fileData.size();
            PackageAndEncode(FileDataPdu(i, fileData), cubeSatEntityId);
            OUTCOME_TRY(SuspendUntilFrameCanBePublished(CancelCondition::receivedFinishedPdu,
                                                        interruptCondition));
            DEBUG_PRINT("Publishing encoded frame with File Data PDU (offset = %d)\n",
                        static_cast<int>(i));
            encodedCfdpFrameMailbox.Overwrite(encodedFrame);
        }
    }
    return outcome_v2::success();
}


// TODO: Refactor
// NOLINTNEXTLINE(*cognitive-complexity)
auto SendAndWaitForAck(Payload const & pdu,
                       DirectiveCode directiveCode,
                       ConditionCode conditionCode,
                       CancelCondition cancelCondition) -> Result<void>
{
    // TODO: Get the fileTransferWindowEnd in there somehow
    for(auto i = 0; i < postiveAckTimerExpirationLimit; ++i)
    {
        // Publish PDU
        PackageAndEncode(pdu, fileDirectivePduType, cubeSatEntityId);
        OUTCOME_TRY(SuspendUntilFrameCanBePublished(cancelCondition, InterruptCondition::never));
        DEBUG_PRINT("Publishing encoded frame with End-Of-File or Finished PDU\n");
        encodedCfdpFrameMailbox.Overwrite(encodedFrame);
        ResumeRfCommunicationThread();

        // Wait for ACK PDU
        auto ackTimerExpirationTime = CurrentRodosTime() + positiveAckTimerInterval;
        while(CurrentRodosTime() < ackTimerExpirationTime)
        {
            (void)receivedPduMailbox.SuspendUntilFullOr(ackTimerExpirationTime);

            // GetReceivedFileDirectivePdu() -> Result<FileDirectivePdu>
            if(receivedPduMailbox.IsEmpty())
            {
                continue;
            }
            auto receivedPdu = receivedPduMailbox.Get().value();
            if(receivedPdu.header.pduType != fileDirectivePduType)
            {
                continue;
            }
            auto parseAsFileDirectiveResult = ParseAsFileDirectivePdu(receivedPdu.dataField);
            if(parseAsFileDirectiveResult.has_error())
            {
                continue;
            }
            auto & fileDirectivePdu = parseAsFileDirectiveResult.value();

            // Check if the right ACK PDU is received
            if(fileDirectivePdu.directiveCode == DirectiveCode::ack)
            {
                auto parseAsAckPduResult = ParseAsAckPdu(fileDirectivePdu.parameterField);
                if(parseAsAckPduResult.has_error())
                {
                    continue;
                }
                auto & ackPdu = parseAsAckPduResult.value();
                auto acknowledgedPduDirectiveCode =
                    DirectiveCode(ackPdu.acknowledgedPduDirectiveCode_.ToUnderlying());
                auto pduIsAcknowledged = acknowledgedPduDirectiveCode == directiveCode
                                     and ackPdu.conditionCode_ == conditionCode
                                     and ackPdu.transactionStatus_ == activeTransactionStatus;
                if(pduIsAcknowledged)
                {
                    DEBUG_PRINT("Received correct ACK PDU\n");
                    return outcome_v2::success();
                }
            }

            // Check(fileDirectivePdu, cancelCondition, interruptCondition) -> Result<void>
            if(cancelCondition == CancelCondition::receivedFinishedPdu
               and fileDirectivePdu.directiveCode == DirectiveCode::finished)
            {
                return ErrorCode::fileTransferCanceled;
            }
            if(cancelCondition == CancelCondition::receivedEofCancelPdu
               and fileDirectivePdu.directiveCode == DirectiveCode::endOfFile)
            {
                auto parseAsEndOfFileResult = ParseAsEndOfFilePdu(fileDirectivePdu.parameterField);
                if(parseAsEndOfFileResult.has_error())
                {
                    continue;
                }
                auto & endOfFilePdu = parseAsEndOfFileResult.value();
                if(cancelCondition == CancelCondition::receivedEofCancelPdu
                   and endOfFilePdu.conditionCode_ != noErrorConditionCode
                   and endOfFilePdu.conditionCode_ != unsupportedChecksumTypeConditionCode)
                {
                    return ErrorCode::fileTransferCanceled;
                }
            }
        }
    }
    return ErrorCode::positiveAckLimitReached;
}
}
}
