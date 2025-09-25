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
// TODO: Maybe rework this into FinishCondition, FinishEarlyCondition, EarlyExitCondition, ...
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


constexpr auto stackSize = 6000U;
// This should be more than enough time to actually send the CFDP frame
constexpr auto fileTransferWindowEndMargin = 5 * s;

auto encodedFrame = std::array<Byte, blockLength>{};
auto frame = tm::TransferFrame(std::span(encodedFrame).first<tm::transferFrameLength>());

// TODO: Should we allow more segments than that?
auto missingFileData =
    etl::vector<SegmentRequest, maxNNaksPerSequence * NakPdu::maxNSegmentRequests>{};


auto SendFile(FileTransferMetadata const & fileTransferMetadata) -> void;
auto ReceiveFile(FileTransferMetadata const & fileTransferMetadata) -> void;
auto ReceiveFirmware(FileTransferMetadata const & fileTransferMetadata) -> void;

// TODO: Think about renaming all the Send*() functions to Publish*()
template<typename Pdu>
    requires std::is_same_v<Pdu, FileDataPdu> or requires { Pdu::directiveCode; }
auto Send(Pdu const & pdu,
          EntityId sourceEntityId,
          CancelCondition cancelCondition,
          InterruptCondition interruptCondition = InterruptCondition::never) -> Result<void>;
auto Send(Payload const & pdu,
          PduType pduType,
          EntityId sourceEntityId,
          CancelCondition cancelCondition,
          InterruptCondition interruptCondition = InterruptCondition::never) -> Result<void>;
auto Send(fs::File const & file, std::uint32_t fileSize) -> Result<void>;
auto Send(fs::File const & file,
          std::uint32_t fileSize,
          std::span<SegmentRequest const> segments,
          InterruptCondition interruptCondition) -> Result<void>;
auto SendAndWaitForAck(EndOfFilePdu const & endOfFilePdu) -> Result<void>;
auto SendAndWaitForAck(FinishedPdu const & finishedPdu) -> Result<void>;
auto SendAndWaitForAck(Payload const & pdu,
                       DirectiveCode directiveCode,
                       ConditionCode conditionCode,
                       CancelCondition cancelCondition,
                       EntityId sourceEntityId) -> Result<void>;
auto Acknowledge(DirectiveCode directiveCode, ConditionCode conditionCode) -> void;

auto SendMissingDataUntilFinished(fs::File const & file, std::uint32_t fileSize) -> Result<void>;
auto SendMissingFileData(fs::File const & file, std::uint32_t fileSize) -> Result<void>;

auto CancelTransfer(auto const & pdu) -> void;
auto AbandonTransfer() -> void;

template<typename Pdu>
    requires std::is_same_v<Pdu, FileDataPdu> or requires { Pdu::directiveCode; }
auto PackageAndEncode(Pdu const & pdu, EntityId sourceEntityId) -> void;
auto PackageAndEncode(Payload const & pdu, PduType pduType, EntityId sourceEntityId) -> void;
auto SuspendUntilFrameCanBePublished(CancelCondition cancelCondition,
                                     InterruptCondition interruptCondition) -> Result<void>;
auto SuspendUntilFileTransferWindowIsOpen() -> void;
auto GetReceivedFileDirectivePdu() -> Result<FileDirectivePdu>;
auto Check(FileDirectivePdu const & fileDirectivePdu,
           CancelCondition cancelCondition,
           InterruptCondition interruptCondition) -> Result<void>;


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
            DEBUG_PRINT_STACK_USAGE();
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
// The high cognitive complexity is misleading because of the OUTCOME_TRY and DEBUG_PRINT macros
// NOLINTNEXTLINE(*cognitive-complexity)
auto SendFile(FileTransferMetadata const & fileTransferMetadata) -> void
{
    std::uint32_t theFileSize = 0;
    auto result = [&](std::uint32_t * fileSize) -> Result<void>
    {
        OUTCOME_TRY(auto file, fs::Open(fileTransferMetadata.sourcePath, LFS_O_RDONLY));
        OUTCOME_TRY(*fileSize, file.Size());
        DEBUG_PRINT("Sending Metadata PDU\n");
        OUTCOME_TRY(Send(
            MetadataPdu(
                *fileSize, fileTransferMetadata.sourcePath, fileTransferMetadata.destinationPath),
            cubeSatEntityId,
            CancelCondition::receivedFinishedPdu));
        OUTCOME_TRY(Send(file, *fileSize));
        OUTCOME_TRY(SendAndWaitForAck(EndOfFilePdu(*fileSize)));
        return SendMissingDataUntilFinished(file, *fileSize);
    }(&theFileSize);
    if(result.has_value())
    {
        DEBUG_PRINT("File transfer finished successfully\n");
        fileTransferStatus.Store(FileTransferStatus::completed);
        return;
    }
    if(result.error() == ErrorCode::fileTransferCanceled)
    {
        DEBUG_PRINT("File transfer canceled by ground station\n");
        fileTransferStatus.Store(FileTransferStatus::canceled);
        return;
    }
    DEBUG_PRINT("Error while sending file: %s\n", ToCZString(result.error()));
    if(IsFileSystemError(result.error()))
    {
        CancelTransfer(EndOfFilePdu(filestoreRejectionConditionCode, theFileSize));
    }
    else if(result.error() == ErrorCode::inactivityDetected)
    {
        CancelTransfer(EndOfFilePdu(inactivityDetectedConditionCode, theFileSize));
    }
    else
    {
        AbandonTransfer();
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
auto Send(Pdu const & pdu,
          EntityId sourceEntityId,
          CancelCondition cancelCondition,
          InterruptCondition interruptCondition) -> Result<void>
{
    if constexpr(std::is_same_v<Pdu, FileDataPdu>)
    {
        return Send(pdu, fileDataPduType, sourceEntityId, cancelCondition, interruptCondition);
    }
    else
    {
        return Send(pdu, fileDirectivePduType, sourceEntityId, cancelCondition, interruptCondition);
    }
}


auto Send(Payload const & pdu,
          PduType pduType,
          EntityId sourceEntityId,
          CancelCondition cancelCondition,
          InterruptCondition interruptCondition) -> Result<void>
{
    PackageAndEncode(pdu, pduType, sourceEntityId);
    OUTCOME_TRY(SuspendUntilFrameCanBePublished(cancelCondition, interruptCondition));
    encodedCfdpFrameMailbox.Overwrite(encodedFrame);
    ResumeRfCommunicationThread();
    return outcome_v2::success();
}


auto Send(fs::File const & file, std::uint32_t fileSize) -> Result<void>
{
    return Send(file,
                fileSize,
                Span(SegmentRequest{.startOffset = 0, .endOffset = fileSize}),
                InterruptCondition::never);
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
            auto chunkSize = std::min<std::size_t>(buffer.size(), fileSize - i);
            auto fileData = std::span(buffer).first(chunkSize);
            OUTCOME_TRY(auto nBytesRead, file.Read(fileData));
            fileData = fileData.first(static_cast<unsigned>(nBytesRead));
            DEBUG_PRINT("Sending File Data PDU (offset = %d)\n", static_cast<int>(i));
            OUTCOME_TRY(Send(FileDataPdu(i, fileData),
                             cubeSatEntityId,
                             CancelCondition::receivedFinishedPdu,
                             interruptCondition));
            i += fileData.size();
        }
    }
    return outcome_v2::success();
}


auto SendAndWaitForAck(EndOfFilePdu const & endOfFilePdu) -> Result<void>
{
    DEBUG_PRINT("Starting pos. ACK procedure for EOF PDU\n");
    return SendAndWaitForAck(endOfFilePdu,
                             endOfFilePdu.directiveCode,
                             endOfFilePdu.conditionCode_,
                             CancelCondition::receivedFinishedPdu,
                             cubeSatEntityId);
}


auto SendAndWaitForAck(FinishedPdu const & finishedPdu) -> Result<void>
{
    DEBUG_PRINT("Starting pos. ACK procedure for Finished PDU\n");
    return SendAndWaitForAck(finishedPdu,
                             finishedPdu.directiveCode,
                             finishedPdu.conditionCode_,
                             CancelCondition::receivedEofCancelPdu,
                             groundStationEntityId);
}


// TODO: Refactor
// NOLINTNEXTLINE(*cognitive-complexity)
auto SendAndWaitForAck(Payload const & pdu,
                       DirectiveCode directiveCode,
                       ConditionCode conditionCode,
                       CancelCondition cancelCondition,
                       EntityId sourceEntityId) -> Result<void>
{
    // TODO: Get the fileTransferWindowEnd in there somehow
    for(auto i = 0; i < postiveAckTimerExpirationLimit; ++i)
    {
        DEBUG_PRINT("Sending EOF or Finished PDU\n");
        OUTCOME_TRY(Send(pdu, fileDirectivePduType, sourceEntityId, cancelCondition));
        auto ackTimerExpirationTime = CurrentRodosTime() + positiveAckTimerInterval;
        while(CurrentRodosTime() < ackTimerExpirationTime)
        {
            (void)receivedPduMailbox.SuspendUntilFullOr(ackTimerExpirationTime);
            auto getFileDirectivePduResult = GetReceivedFileDirectivePdu();
            if(getFileDirectivePduResult.has_error())
            {
                continue;
            }
            auto & fileDirectivePdu = getFileDirectivePduResult.value();
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
            OUTCOME_TRY(Check(fileDirectivePdu, cancelCondition, InterruptCondition::never));
        }
    }
    return ErrorCode::positiveAckLimitReached;
}


auto Acknowledge(DirectiveCode directiveCode, ConditionCode conditionCode) -> void
{
    auto sourceEndityId =
        directiveCode == DirectiveCode::finished ? cubeSatEntityId : groundStationEntityId;
    PackageAndEncode(AckPdu(directiveCode, conditionCode, activeTransactionStatus), sourceEndityId);
    (void)encodedCfdpFrameMailbox.Get();  // Empty the mailbox
    SuspendUntilFileTransferWindowIsOpen();
    DEBUG_PRINT("Sending ACK PDU\n");
    encodedCfdpFrameMailbox.Overwrite(encodedFrame);
    ResumeRfCommunicationThread();  // Immediately send the PDU
}


// TODO: Refactor
// NOLINTNEXTLINE(*cognitive-complexity)
auto SendMissingDataUntilFinished(fs::File const & file, std::uint32_t fileSize) -> Result<void>
{
    ResumeRfCommunicationThread();  // To go into RX mode again
    missingFileData.clear();
    // TODO: Get the fileTransferWindowEnd in there somehow
    auto timeLimit = CurrentRodosTime() + transactionInactivityLimit;
    for(auto nNaksInSequence = 0;;)
    {
        auto suspendResult = receivedPduMailbox.SuspendUntilFullOr(timeLimit);
        if(suspendResult.has_error() and suspendResult.error() == ErrorCode::timeout)
        {
            if(nNaksInSequence == 0)
            {
                return ErrorCode::inactivityDetected;
            }
            OUTCOME_TRY(SendMissingFileData(file, fileSize));
            nNaksInSequence = 0;
            timeLimit = CurrentRodosTime() + transactionInactivityLimit;
            continue;
        }
        auto getFileDirectivePduResult = GetReceivedFileDirectivePdu();
        if(getFileDirectivePduResult.has_error())
        {
            continue;
        }
        auto & fileDirectivePdu = getFileDirectivePduResult.value();

        // Handle NAK PDU
        if(fileDirectivePdu.directiveCode == DirectiveCode::nak)
        {
            auto parseAsNakPduResult = ParseAsNakPdu(fileDirectivePdu.parameterField);
            if(parseAsNakPduResult.has_error())
            {
                continue;
            }
            ++nNaksInSequence;
            auto & nakPdu = parseAsNakPduResult.value();
            missingFileData.insert(missingFileData.end(),
                                   nakPdu.segmentRequests_.begin(),
                                   nakPdu.segmentRequests_.end());
            auto isFinalNakInSequence =
                nakPdu.endOfScope_ == fileSize or nNaksInSequence == maxNNaksPerSequence;
            if(isFinalNakInSequence)
            {
                OUTCOME_TRY(SendMissingFileData(file, fileSize));
                nNaksInSequence = 0;
                timeLimit = CurrentRodosTime() + transactionInactivityLimit;
            }
            else
            {
                timeLimit = CurrentRodosTime() + nakSequenceTimeout;
            }
            continue;
        }

        // Handle Finished PDU
        if(fileDirectivePdu.directiveCode == DirectiveCode::finished)
        {
            auto parseAsFinishedPduResult = ParseAsFinishedPdu(fileDirectivePdu.parameterField);
            if(parseAsFinishedPduResult.has_error())
            {
                continue;
            }
            auto & finishedPdu = parseAsFinishedPduResult.value();
            Acknowledge(finishedPdu.directiveCode, finishedPdu.conditionCode_);
            if(finishedPdu.conditionCode_ != noErrorConditionCode
               and finishedPdu.conditionCode_ != unsupportedChecksumTypeConditionCode)
            {
                return ErrorCode::fileTransferCanceled;
            }
            return outcome_v2::success();
        }
    }
}


auto SendMissingFileData(fs::File const & file, std::uint32_t fileSize) -> Result<void>
{
    DEBUG_PRINT("Sending %d missing file data\n", static_cast<int>(missingFileData.size()));
    (void)encodedCfdpFrameMailbox.Get();  // Empty the mailbox to immediately send the new data
    auto sendResult = Send(file, fileSize, missingFileData, InterruptCondition::receivedNakPdu);
    missingFileData.clear();
    if(sendResult.has_error() and sendResult.error() != ErrorCode::fileTransferInterrupted)
    {
        return sendResult.error();
    }
    return outcome_v2::success();
}


auto CancelTransfer(auto const & pdu) -> void
{
    DEBUG_PRINT("  -> canceling transfer\n");
    fileTransferStatus.Store(FileTransferStatus::canceled);
    auto sendAndWaitResult = SendAndWaitForAck(pdu);
    if(sendAndWaitResult.has_error())
    {
        DEBUG_PRINT("Error while canceling transfer: %s\n", ToCZString(sendAndWaitResult.error()));
        AbandonTransfer();
    }
}


auto AbandonTransfer() -> void
{
    DEBUG_PRINT("  -> abandoning transfer\n");
    fileTransferStatus.Store(FileTransferStatus::abandoned);
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


auto SuspendUntilFrameCanBePublished(CancelCondition cancelCondition,
                                     InterruptCondition interruptCondition) -> Result<void>
{
    // TODO: We do not check the received PDUs if the encodedCfdpFrameMailbox is not full
    while(encodedCfdpFrameMailbox.IsFull())
    {
        // TODO: Think about the correct reactivation time for all suspend functions
        (void)encodedCfdpFrameMailbox.SuspendUntilEmptyOr(endOfTime);
        auto getFileDirectivePduResult = GetReceivedFileDirectivePdu();
        if(getFileDirectivePduResult.has_error())
        {
            continue;
        }
        auto & fileDirectivePdu = getFileDirectivePduResult.value();
        OUTCOME_TRY(Check(fileDirectivePdu, cancelCondition, interruptCondition));
    }
    SuspendUntilFileTransferWindowIsOpen();
    return outcome_v2::success();
}


auto SuspendUntilFileTransferWindowIsOpen() -> void
{
    while(CurrentRodosTime()
          > persistentVariables.Load<"fileTransferWindowEnd">() + fileTransferWindowEndMargin)
    {
        SuspendUntil(endOfTime);
    }
}


auto GetReceivedFileDirectivePdu() -> Result<FileDirectivePdu>
{
    OUTCOME_TRY(auto receivedPdu, receivedPduMailbox.Get());
    if(receivedPdu.header.pduType != fileDirectivePduType)
    {
        return ErrorCode::wrongPduType;
    }
    return ParseAsFileDirectivePdu(receivedPdu.dataField);
}


// Return fileTransferCanceled or fileTransferInterrupted depending on the conditions. If the file
// transfer is canceled, also acknowledge the cancellation right away.
auto Check(FileDirectivePdu const & fileDirectivePdu,
           CancelCondition cancelCondition,
           InterruptCondition interruptCondition) -> Result<void>
{
    if(cancelCondition == CancelCondition::receivedFinishedPdu
       and fileDirectivePdu.directiveCode == DirectiveCode::finished)
    {
        auto parseAsFinishedPduResult = ParseAsFinishedPdu(fileDirectivePdu.parameterField);
        if(parseAsFinishedPduResult.has_error())
        {
            return outcome_v2::success();
        }
        auto & finishedPdu = parseAsFinishedPduResult.value();
        Acknowledge(finishedPdu.directiveCode, finishedPdu.conditionCode_);
        return ErrorCode::fileTransferCanceled;
    }
    if((cancelCondition == CancelCondition::receivedEofCancelPdu
        or interruptCondition == InterruptCondition::receivedEofNoErrorPdu)
       and fileDirectivePdu.directiveCode == DirectiveCode::endOfFile)
    {
        auto parseAsEndOfFileResult = ParseAsEndOfFilePdu(fileDirectivePdu.parameterField);
        if(parseAsEndOfFileResult.has_error())
        {
            return outcome_v2::success();
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
            Acknowledge(endOfFilePdu.directiveCode, endOfFilePdu.conditionCode_);
            return ErrorCode::fileTransferCanceled;
        }
    }
    if(interruptCondition == InterruptCondition::receivedNakPdu
       and fileDirectivePdu.directiveCode == DirectiveCode::nak)
    {
        return ErrorCode::fileTransferInterrupted;
    }
    return outcome_v2::success();
}
}
}
