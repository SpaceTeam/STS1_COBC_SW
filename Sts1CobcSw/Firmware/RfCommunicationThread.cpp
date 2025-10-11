#include <Sts1CobcSw/Firmware/RfCommunicationThread.hpp>

#include <Sts1CobcSw/ChannelCoding/ChannelCoding.hpp>
#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Edu/ProgramQueue.hpp>
#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/ErrorDetectionAndCorrection/EdacVariable.hpp>
#include <Sts1CobcSw/FileSystem/DirectoryIterator.hpp>
#include <Sts1CobcSw/FileSystem/File.hpp>
#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/Firmware/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Firmware/FileTransferThread.hpp>
#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/FirmwareManagement/FirmwareManagement.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/FramRingArray.hpp>
#include <Sts1CobcSw/FramSections/FramVector.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Mailbox/Mailbox.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RealTime/RealTime.hpp>
#include <Sts1CobcSw/Rf/Rf.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/Payload.hpp>
#include <Sts1CobcSw/RfProtocols/ProtocolDataUnitHeader.hpp>
#include <Sts1CobcSw/RfProtocols/ProtocolDataUnits.hpp>
#include <Sts1CobcSw/RfProtocols/Reports.hpp>
#include <Sts1CobcSw/RfProtocols/Requests.hpp>
#include <Sts1CobcSw/RfProtocols/SpacePacket.hpp>
#include <Sts1CobcSw/RfProtocols/TcSpacePacketSecondaryHeader.hpp>
#include <Sts1CobcSw/RfProtocols/TcTransferFrame.hpp>
#include <Sts1CobcSw/RfProtocols/TmTransferFrame.hpp>
#include <Sts1CobcSw/RfProtocols/Vocabulary.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryMemory.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Vocabulary/FileTransfer.hpp>
#include <Sts1CobcSw/Vocabulary/Ids.hpp>  // IWYU pragma: keep
#include <Sts1CobcSw/Vocabulary/MessageTypeIdFields.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>
#include <Sts1CobcSw/WatchdogTimers/WatchdogTimers.hpp>

#include <littlefs/lfs.h>
#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/equality.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/string.h>
#include <etl/vector.h>

#include <algorithm>
#include <array>
#include <cinttypes>  // IWYU pragma: keep
#include <climits>
#include <compare>
#include <cstdint>
#include <iterator>
#include <span>
#include <type_traits>
#include <utility>


namespace sts1cobcsw
{
namespace
{
constexpr auto stackSize = 6000;
constexpr auto rxTimeoutAfterTelemetryRecord = 5 * s;
constexpr auto rxTimeoutForAdditionalData = 3 * s;
constexpr auto minRxTimeout = 1 * s;
constexpr auto estimatedMaxDataProcessingDuration = 100 * ms;
// The ground station needs some time to switch from TX to RX so we need to wait for that when
// switching from RX to TX
constexpr auto rxToTxSwitchDuration = 300 * ms;
constexpr auto maxNFramesToSendContinously = rf::maxTxDataLength / fullyEncodedFrameLength;

auto tcBuffer = std::array<Byte, blockLength>{};
auto tmBuffer = std::array<Byte, channelAccessDataUnitLength>{};
auto tmBlock = std::span(tmBuffer).subspan<attachedSynchMarkerLength, blockLength>();
auto tmFrame = tm::TransferFrame(tmBlock.first<tm::transferFrameLength>());
auto lastRxTime = RodosTime(0);
std::uint16_t nFramesToSend = 0U;
std::uint16_t nSentFrames = 0U;


auto SuspendUntilNewTelemetryRecordIsAvailable() -> void;
auto EstimateDataHandlingDuration() -> Duration;
auto ReceiveAndHandleData(Duration rxTimeout) -> Result<void>;
auto SendCfdpFrames() -> void;
auto HandleReceivedData() -> void;
auto HandleCfdpFrame(tc::TransferFrame const & frame) -> void;
auto PduHeaderMatchesTransferInfo(ProtocolDataUnitHeader const & pduHeader,
                                  std::uint16_t transactionSequenceNumber,
                                  FileTransferStatus fileTransferStatus) -> bool;
auto HandleRequestFrame(tc::TransferFrame const & frame) -> void;
auto Handle(Request const & request, RequestId const & requestId) -> void;

template<auto parseFunction>
auto VerifyAndHandle(Request const & request, RequestId const & requestId) -> void;

// Only those requests which answer with a successful/failed verification report need the requestId,
// but to make our life in VerifyAndHandle() easier, we pass it to all handlers.
auto Handle(LoadRawMemoryDataAreasRequest const & request, RequestId const & requestId) -> void;
auto Handle(DumpRawMemoryDataRequest const & request, RequestId const & requestId) -> void;
auto Handle(PerformAFunctionRequest const & request, RequestId const & requestId) -> void;
auto Handle(ReportParameterValuesRequest const & request, RequestId const & requestId) -> void;
auto Handle(SetParameterValuesRequest const & request, RequestId const & requestId) -> void;
auto Handle(DeleteAFileRequest const & request, RequestId const & requestId) -> void;
auto Handle(ReportTheAttributesOfAFileRequest const & request, RequestId const & requestId) -> void;
auto Handle(SummaryReportTheContentOfARepositoryRequest const & request,
            RequestId const & requestId) -> void;
auto Handle(CopyAFileRequest const & request, RequestId const & requestId) -> void;

template<auto parseFunction>
auto VerifyAndHandle(PerformAFunctionRequest const & request, RequestId const & requestId) -> void;

// As above, not all functions need the requestId, but it's easier if we pass it to all handlers
auto Handle(ReportHousekeepingParameterReportFunction const & function, RequestId const & requestId)
    -> void;
auto Handle(EnableFileTransferFunction const & function, RequestId const & requestId) -> void;
auto Handle(SynchronizeTimeFunction const & function, RequestId const & requestId) -> void;
auto Handle(UpdateEduQueueFunction const & function, RequestId const & requestId) -> void;
auto Handle(SetActiveFirmwareFunction const & function, RequestId const & requestId) -> void;
auto Handle(SetBackupFirmwareFunction const & function, RequestId const & requestId) -> void;
auto Handle(CheckFirmwareIntegrityFunction const & function, RequestId const & requestId) -> void;

[[nodiscard]] auto ToRequestId(SpacePacketPrimaryHeader const & header) -> RequestId;
[[nodiscard]] auto GetValue(Parameter::Id parameterId) -> Parameter::Value;
auto Set(Parameter parameter) -> void;
[[nodiscard]] auto ValidateAndBuildFileTransferMetadata(CopyAFileRequest const & request)
    -> Result<FileTransferMetadata>;
[[nodiscard]] auto GetPartitionId(fs::Path const & filePath) -> Result<PartitionId>;

// Must be called before SendAndContinue()
auto SetTxDataLength(std::uint16_t nFrames) -> void;
auto SendAndWait(Payload const & report) -> void;
auto SendAndContinue(Payload const & report) -> void;
auto PackageAndEncode(Payload const & report) -> void;
auto SendAndWait(std::span<Byte const, channelAccessDataUnitLength> channelAccessDataUnit) -> void;
auto SendAndContinue(std::span<Byte const, channelAccessDataUnitLength> channelAccessDataUnit)
    -> void;
auto SuspendUntilEarliestTxTime() -> void;
// Must be called after SendAndContinue()
auto FinalizeTransmission() -> void;


class RfCommunicationThread : public RODOS::StaticThread<stackSize>
{
public:
    RfCommunicationThread() : StaticThread("RfCommunicationThread", rfCommunicationThreadPriority)
    {}


private:
    auto init() -> void override
    {
        if constexpr(attachedSynchMarkerLength == attachedSynchMarker.size())
        {
            std::ranges::copy(attachedSynchMarker, tmBuffer.begin());
        }
    }


    auto run() -> void override
    {
        SuspendFor(totalStartupTestTimeout);  // Wait for the startup tests to complete
        DEBUG_PRINT("Starting RF communication thread\n");
        auto moreDataShouldBeReceived = false;
        while(true)
        {
            if(telemetryRecordMailbox.IsFull())
            {
                DEBUG_PRINT("Sending housekeeping parameter report\n");
                SendAndWait(HousekeepingParameterReport(telemetryRecordMailbox.Get().value()));
                DEBUG_PRINT("Receiving for %" PRIi64 " s\n", rxTimeoutAfterTelemetryRecord / s);
                auto receiveResult = ReceiveAndHandleData(rxTimeoutAfterTelemetryRecord);
                moreDataShouldBeReceived = receiveResult.has_value();
                DEBUG_PRINT_STACK_USAGE();
                continue;
            }
            if(encodedCfdpFrameMailbox.IsFull())
            {
                SendCfdpFrames();
                SuspendUntilNewTelemetryRecordIsAvailable();
                continue;
            }
            if(moreDataShouldBeReceived)
            {
                auto remainingRxDuration = nextTelemetryRecordTimeMailbox.Peek().value()
                                         - CurrentRodosTime() - EstimateDataHandlingDuration();
                if(remainingRxDuration > minRxTimeout)
                {
                    auto rxTimeout = std::min(remainingRxDuration, rxTimeoutForAdditionalData);
                    DEBUG_PRINT("Receiving for %" PRIi64 " s\n", rxTimeout / s);
                    auto receiveResult = ReceiveAndHandleData(rxTimeout);
                    moreDataShouldBeReceived = receiveResult.has_value();
                    continue;
                }
            }
            SuspendUntilNewTelemetryRecordIsAvailable();
        }
    }
} rfCommunicationThread;
}


auto ResumeRfCommunicationThread() -> void
{
    rfCommunicationThread.resume();
}


namespace
{
auto SuspendUntilNewTelemetryRecordIsAvailable() -> void
{
    (void)telemetryRecordMailbox.SuspendUntilFullOr(endOfTime);
}


auto EstimateDataHandlingDuration() -> Duration
{
    // Worst case is that we receive a request and need to send two reports in response
    auto frameSendDuration = fullyEncodedFrameLength * CHAR_BIT * s / rf::GetTxDataRate();
    return 2 * frameSendDuration + estimatedMaxDataProcessingDuration;
}


auto ReceiveAndHandleData(Duration rxTimeout) -> Result<void>
{
    auto nReceivedBytes = rf::Receive(tcBuffer, rxTimeout);
    lastRxTime = CurrentRodosTime();
    if(nReceivedBytes < tcBuffer.size())
    {
        return ErrorCode::timeout;
    }
    HandleReceivedData();
    return outcome_v2::success();
}


auto SendCfdpFrames() -> void
{
    static constexpr auto sendWindowMargin = 1 * s;
    // We know that the nextTelemetryRecordTimeMailbox is never empty here, because the telemetry
    // thread has a higher priority and will always write to it before we read it.
    auto sendWindowEnd = std::min(nextTelemetryRecordTimeMailbox.Peek().value(),
                                  persistentVariables.Load<"fileTransferWindowEnd">())
                       - sendWindowMargin;
    auto frameSendDuration = fullyEncodedFrameLength * CHAR_BIT * s / rf::GetTxDataRate();
    auto nFrames =
        static_cast<std::uint16_t>((CurrentRodosTime() - sendWindowEnd) / frameSendDuration);
    if(nFrames > 0)
    {
        SetTxDataLength(nFrames);
    }
    while(encodedCfdpFrameMailbox.IsFull()
          and CurrentRodosTime() + frameSendDuration < sendWindowEnd)
    {
        // This wakes up the file transfer thread if it is waiting to send a new CFDP frame
        std::ranges::copy(encodedCfdpFrameMailbox.Get().value(), tmBlock.begin());
        // Write the attached sync marker to the beginning of the buffer every time because the
        // constant is stored in flash and therefore not corrupted as fast as the buffer in RAM.
        if constexpr(attachedSynchMarkerLength == attachedSynchMarker.size())
        {
            std::ranges::copy(attachedSynchMarker, tmBuffer.begin());
        }
        SendAndContinue(tmBuffer);
        // In case radiation affects the while condition, we add this additional check to break once
        // a new telemetry record is available
        if(telemetryRecordMailbox.IsFull())
        {
            return;
        }
    }
    // TODO: It looks like we are missing a FinalizeTransmission() are something here. Having just a
    // SendAndContinue() seems wrong.
    ResumeFileTransferThread();
}


auto HandleReceivedData() -> void
{
    rdt::Feed();
    persistentVariables.Store<"nResetsSinceRf">(0);
    auto result = [&]() -> Result<void>
    {
        auto decodeResult = tc::Decode(tcBuffer);
        if(decodeResult.has_error())
        {
            persistentVariables.Increment<"nUncorrectableUplinkErrors">();
            return decodeResult.error();
        }
        persistentVariables.Add<"nCorrectableUplinkErrors">(
            static_cast<std::uint16_t>(decodeResult.value()));
        OUTCOME_TRY(auto tcFrame,
                    tc::ParseAsTransferFrame(std::span(tcBuffer).first<tc::transferFrameLength>()));
        persistentVariables.Store<"lastFrameSequenceNumber">(
            tcFrame.primaryHeader.frameSequenceNumber);
        if(tcFrame.primaryHeader.vcid == cfdpVcid)
        {
            HandleCfdpFrame(tcFrame);
        }
        else
        {
            HandleRequestFrame(tcFrame);
        }
        return outcome_v2::success();
    }();
    if(result.has_error())
    {
        DEBUG_PRINT("Error in HandleReceivedData(): %s\n", ToCZString(result.error()));
        persistentVariables.Increment<"nBadTransferFrames">();
    }
}


auto HandleCfdpFrame(tc::TransferFrame const & frame) -> void
{
    auto transferStatus = fileTransferStatus.Load();
    if(transferStatus != FileTransferStatus::sending
       and transferStatus != FileTransferStatus::receiving
       and transferStatus != FileTransferStatus::canceled)
    {
        DEBUG_PRINT("Discarding CFDP frame because no file transfer is ongoing\n");
        return;
    }
    auto parseAsProtocolDataUnitResult = ParseAsProtocolDataUnit(frame.dataField);
    if(parseAsProtocolDataUnitResult.has_error())
    {
        DEBUG_PRINT("Error parsing as Protocol Data Unit: %s\n",
                    ToCZString(parseAsProtocolDataUnitResult.error()));
        return;
    }
    auto const & pdu = parseAsProtocolDataUnitResult.value();
    auto sequenceNumber = transactionSequenceNumber.Load();
    if(transferStatus == FileTransferStatus::receiving
       and sequenceNumber == unknownTransactionSequenceNumber)
    {
        sequenceNumber = pdu.header.transactionSequenceNumber;
        transactionSequenceNumber.Store(sequenceNumber);
        DEBUG_PRINT("Set trans. seq. no. to %u\n", sequenceNumber);
    }
    if(not PduHeaderMatchesTransferInfo(pdu.header, sequenceNumber, transferStatus))
    {
        return;
    }
    // This wakes up the file transfer thread if it is waiting for a new PDU
    receivedPduMailbox.Overwrite(pdu);
    SuspendUntilNewTelemetryRecordIsAvailable();
}


// NOLINTNEXTLINE(*cognitive-complexity)
auto PduHeaderMatchesTransferInfo(ProtocolDataUnitHeader const & pduHeader,
                                  std::uint16_t transactionSequenceNumber,
                                  FileTransferStatus fileTransferStatus) -> bool
{
    if(pduHeader.transactionSequenceNumber != transactionSequenceNumber)
    {
        DEBUG_PRINT("Discarding CFDP frame because transaction sequence number is wrong\n");
        DEBUG_PRINT("  received = %d, expected = %d\n",
                    static_cast<int>(pduHeader.transactionSequenceNumber),
                    static_cast<int>(transactionSequenceNumber));
        return false;
    }
    auto directionAndIdsAreCorrect = pduHeader.sourceEntityId != pduHeader.destinationEntityId
                                 and (fileTransferStatus == FileTransferStatus::canceled
                                      or (fileTransferStatus == FileTransferStatus::sending
                                          and pduHeader.direction == towardsFileSenderDirection
                                          and pduHeader.sourceEntityId == cubeSatEntityId)
                                      or (fileTransferStatus == FileTransferStatus::receiving
                                          and pduHeader.direction == towardsFileReceiverDirection
                                          and pduHeader.sourceEntityId == groundStationEntityId));
    if(not directionAndIdsAreCorrect)
    {
        DEBUG_PRINT("Discarding CFDP frame because direction or entity IDs are wrong\n");
        DEBUG_PRINT("  direction:      received = %d, expected = %d\n",
                    value_of(pduHeader.direction).ToUnderlying(),
                    fileTransferStatus == FileTransferStatus::sending
                        ? value_of(towardsFileSenderDirection).ToUnderlying()
                        : value_of(towardsFileReceiverDirection).ToUnderlying());
        DEBUG_PRINT("  source ID:      received = 0x%02x, expected = 0x%02x\n",
                    pduHeader.sourceEntityId.Value(),
                    fileTransferStatus == FileTransferStatus::sending
                        ? cubeSatEntityId.Value()
                        : groundStationEntityId.Value());
        DEBUG_PRINT("  destination ID: received = 0x%02x, expected = 0x%02x\n",
                    pduHeader.destinationEntityId.Value(),
                    fileTransferStatus == FileTransferStatus::sending
                        ? groundStationEntityId.Value()
                        : cubeSatEntityId.Value());
        return false;
    }
    return true;
}


auto HandleRequestFrame(tc::TransferFrame const & frame) -> void
{
    auto parseAsSpacePacketResult = ParseAsSpacePacket(frame.dataField);
    if(parseAsSpacePacketResult.has_error())
    {
        DEBUG_PRINT("Error parsing as Space Packet: %s\n",
                    ToCZString(parseAsSpacePacketResult.error()));
        return;
    }
    auto const & spacePacket = parseAsSpacePacketResult.value();
    auto requestId = ToRequestId(spacePacket.primaryHeader);
    auto parseAsRequestResult = ParseAsRequest(spacePacket.dataField);
    if(parseAsRequestResult.has_error())
    {
        persistentVariables.Store<"lastMessageTypeIdWasInvalid">(
            parseAsRequestResult.error() == ErrorCode::invalidMessageTypeId);
        DEBUG_PRINT("Failed acceptance of request: %s\n", ToCZString(parseAsRequestResult.error()));
        SendAndWait(FailedAcceptanceVerificationReport(requestId, parseAsRequestResult.error()));
        return;
    }
    auto const & request = parseAsRequestResult.value();
    persistentVariables.Store<"lastMessageTypeIdWasInvalid">(false);
    persistentVariables.Store<"lastMessageTypeId">(
        request.packetSecondaryHeader.messageTypeId.Value());
    Handle(request, requestId);
}


auto Handle(Request const & request, RequestId const & requestId) -> void
{
    auto const & messageTypeId = request.packetSecondaryHeader.messageTypeId;
    if(messageTypeId == LoadRawMemoryDataAreasRequest::id)
    {
        VerifyAndHandle<ParseAsLoadRawMemoryDataAreasRequest>(request, requestId);
    }
    else if(messageTypeId == DumpRawMemoryDataRequest::id)
    {
        VerifyAndHandle<ParseAsDumpRawMemoryDataRequest>(request, requestId);
    }
    else if(messageTypeId == PerformAFunctionRequest::id)
    {
        VerifyAndHandle<ParseAsPerformAFunctionRequest>(request, requestId);
    }
    else if(messageTypeId == ReportParameterValuesRequest::id)
    {
        VerifyAndHandle<ParseAsReportParameterValuesRequest>(request, requestId);
    }
    else if(messageTypeId == SetParameterValuesRequest::id)
    {
        VerifyAndHandle<ParseAsSetParameterValuesRequest>(request, requestId);
    }
    else if(messageTypeId == DeleteAFileRequest::id)
    {
        VerifyAndHandle<ParseAsDeleteAFileRequest>(request, requestId);
    }
    else if(messageTypeId == ReportTheAttributesOfAFileRequest::id)
    {
        VerifyAndHandle<ParseAsReportTheAttributesOfAFileRequest>(request, requestId);
    }
    else if(messageTypeId == SummaryReportTheContentOfARepositoryRequest::id)
    {
        VerifyAndHandle<ParseAsSummaryReportTheContentOfARepositoryRequest>(request, requestId);
    }
    else if(messageTypeId == CopyAFileRequest::id)
    {
        VerifyAndHandle<ParseAsCopyAFileRequest>(request, requestId);
    }
}


template<auto parseFunction>
auto VerifyAndHandle(Request const & request, RequestId const & requestId) -> void
{
    auto parseResult = parseFunction(request.applicationData);
    if(parseResult.has_error())
    {
        DEBUG_PRINT("Failed acceptance of request: %s\n", ToCZString(parseResult.error()));
        SendAndWait(FailedAcceptanceVerificationReport(requestId, parseResult.error()));
        return;
    }
    // PerformAFunctionRequest has one more level of parsing to do before the successful acceptance
    // verification report can be sent
    if constexpr(not std::is_same_v<decltype(parseFunction),
                                    decltype(&ParseAsPerformAFunctionRequest)>)
    {
        DEBUG_PRINT("Successfully accepted request\n");
        SendAndWait(SuccessfulAcceptanceVerificationReport(requestId));
    }
    Handle(parseResult.value(), requestId);
}


auto Handle(LoadRawMemoryDataAreasRequest const & request, RequestId const & requestId) -> void
{
    static constexpr auto timeout = 100 * ms;
    fram::WriteTo(request.startAddress, request.data, timeout);
    DEBUG_PRINT("Successfully loaded raw memory data areas\n");
    SendAndWait(SuccessfulCompletionOfExecutionVerificationReport(requestId));
}


auto Handle(DumpRawMemoryDataRequest const & request, [[maybe_unused]] RequestId const & requestId)
    -> void
{
    auto dumpedData = etl::vector<Byte, maxDumpedDataLength>{};
    SetTxDataLength(request.nDataAreas);
    for(auto && dataArea : request.dataAreas)
    {
        dumpedData.uninitialized_resize(dataArea.length);
        static constexpr auto timeout = 100 * ms;
        fram::ReadFrom(dataArea.startAddress, std::span(dumpedData), timeout);
        DEBUG_PRINT("Sending dumped raw memory data report\n");
        SendAndContinue(DumpedRawMemoryDataReport(1, dataArea.startAddress, dumpedData));
        if(telemetryRecordMailbox.IsFull())
        {
            break;
        }
    }
    FinalizeTransmission();
}


auto Handle(PerformAFunctionRequest const & request, RequestId const & requestId) -> void
{
    switch(request.functionId)
    {
        case FunctionId::stopAntennaDeployment:
            persistentVariables.Store<"antennasShouldBeDeployed">(false);
            DEBUG_PRINT("Stopped antenna deployment\n");
            SendAndWait(SuccessfulCompletionOfExecutionVerificationReport(requestId));
            return;
        case FunctionId::requestHousekeepingParameterReports:
            VerifyAndHandle<ParseAsReportHousekeepingParameterReportFunction>(request, requestId);
            return;
        case FunctionId::disableCubeSatTx:
            rf::DisableTx();
            DEBUG_PRINT("Disabled CubeSat TX\n");
            return;
        case FunctionId::enableCubeSatTx:
            rf::EnableTx();
            DEBUG_PRINT("Enabled CubeSat TX\n");
            SendAndWait(SuccessfulCompletionOfExecutionVerificationReport(requestId));
            return;
        case FunctionId::resetNow:
            RODOS::hwResetAndReboot();
            return;
        case FunctionId::enableFileTransfer:
            VerifyAndHandle<ParseAsEnableFileTransferFunction>(request, requestId);
            return;
        case FunctionId::synchronizeTime:
            VerifyAndHandle<ParseAsSynchronizeTimeFunction>(request, requestId);
            return;
        case FunctionId::updateEduQueue:
            VerifyAndHandle<ParseAsUpdateEduQueueFunction>(request, requestId);
            return;
        case FunctionId::setActiveFirmware:
            VerifyAndHandle<ParseAsSetActiveFirmwareFunction>(request, requestId);
            return;
        case FunctionId::setBackupFirmware:
            VerifyAndHandle<ParseAsSetBackupFirmwareFunction>(request, requestId);
            return;
        case FunctionId::checkFirmwareIntegrity:
            VerifyAndHandle<ParseAsCheckFirmwareIntegrityFunction>(request, requestId);
            return;
    }
}


auto Handle(ReportParameterValuesRequest const & request,
            [[maybe_unused]] RequestId const & requestId) -> void
{
    auto parameters = etl::vector<Parameter, maxNParameters>{};
    for(auto parameterId : request.parameterIds)
    {
        parameters.push_back({parameterId, GetValue(parameterId)});
    }
    DEBUG_PRINT("Sending parameter value report\n");
    SendAndWait(ParameterValueReport(parameters));
}


auto Handle(SetParameterValuesRequest const & request, [[maybe_unused]] RequestId const & requestId)
    -> void
{
    auto parameters = etl::vector<Parameter, maxNParameters>{};
    for(auto parameter : request.parameters)
    {
        Set(parameter);
        parameters.push_back({parameter.id, GetValue(parameter.id)});
    }
    DEBUG_PRINT("Sending parameter value report\n");
    SendAndWait(ParameterValueReport(parameters));
}


auto Handle(DeleteAFileRequest const & request, RequestId const & requestId) -> void
{
    auto result = fs::Remove(request.filePath);
    if(result.has_error())
    {
        DEBUG_PRINT(
            "Failed to delete file %s: %s\n", request.filePath.c_str(), ToCZString(result.error()));
        SendAndWait(FailedCompletionOfExecutionVerificationReport(requestId, result.error()));
        return;
    }
    DEBUG_PRINT("Successfully deleted file %s\n", request.filePath.c_str());
    SendAndWait(SuccessfulCompletionOfExecutionVerificationReport(requestId));
}


auto Handle(ReportTheAttributesOfAFileRequest const & request, RequestId const & requestId) -> void
{
    auto result = [&]() -> Result<void>
    {
        OUTCOME_TRY(auto isLocked, fs::IsLocked(request.filePath));
        OUTCOME_TRY(auto fileSize, fs::FileSize(request.filePath));
        auto lockState = isLocked ? LockState::locked : LockState::unlocked;
        DEBUG_PRINT("Sending file attribute report for %s\n", request.filePath.c_str());
        SendAndWait(FileAttributeReport(request.filePath, fileSize, lockState));
        return outcome_v2::success();
    }();
    if(result.has_error())
    {
        DEBUG_PRINT("Failed to report attributes of file %s: %s\n",
                    request.filePath.c_str(),
                    ToCZString(result.error()));
        SendAndWait(FailedCompletionOfExecutionVerificationReport(requestId, result.error()));
    }
}


// NOLINTNEXTLINE(*cognitive-complexity)
auto Handle(SummaryReportTheContentOfARepositoryRequest const & request,
            RequestId const & requestId) -> void
{
    auto result = [&]() -> Result<void>
    {
        OUTCOME_TRY(auto iterator, fs::MakeIterator(request.repositoryPath));
        auto nObjects = static_cast<std::uint8_t>(std::distance(iterator, iterator.end()));
        OUTCOME_TRY(iterator, fs::MakeIterator(request.repositoryPath));
        static constexpr auto maxNObjectsPerPacket =
            RepositoryContentSummaryReport::maxNObjectsPerPacket;
        auto objects = etl::vector<FileSystemObject, maxNObjectsPerPacket>{};
        // Round up to get the number of frames required to send all objects
        SetTxDataLength(static_cast<std::uint16_t>((nObjects + maxNObjectsPerPacket - 1)
                                                   / maxNObjectsPerPacket));
        while(iterator != iterator.end())
        {
            auto dereferenceResult = *iterator;
            if(dereferenceResult.has_error())
            {
                break;
            }
            auto const & directoryInfo = dereferenceResult.value();
            auto objectType = directoryInfo.type == fs::EntryType::file
                                ? FileSystemObject::Type::file
                                : FileSystemObject::Type::directory;
            objects.push_back({objectType, directoryInfo.name});
            ++iterator;
            if(objects.full() or iterator == iterator.end())
            {
                DEBUG_PRINT("Sending repository content summary report for %s\n",
                            request.repositoryPath.c_str());
                SendAndContinue(
                    RepositoryContentSummaryReport(request.repositoryPath, nObjects, objects));
                if(telemetryRecordMailbox.IsFull())
                {
                    break;
                }
                objects.clear();
            }
        }
        FinalizeTransmission();
        return outcome_v2::success();
    }();
    if(result.has_error())
    {
        DEBUG_PRINT("Failed to report content of repository %s: %s\n",
                    request.repositoryPath.c_str(),
                    ToCZString(result.error()));
        SendAndWait(FailedCompletionOfExecutionVerificationReport(requestId, result.error()));
    }
}


auto Handle(CopyAFileRequest const & request, RequestId const & requestId) -> void
{
    auto fileTransferMetadataResult = ValidateAndBuildFileTransferMetadata(request);
    if(fileTransferMetadataResult.has_error())
    {
        DEBUG_PRINT("Failed to initiate copying file '%s' to '%s': %s\n",
                    request.sourceFilePath.c_str(),
                    request.targetFilePath.c_str(),
                    ToCZString(fileTransferMetadataResult.error()));
        SendAndWait(FailedCompletionOfExecutionVerificationReport(
            requestId, fileTransferMetadataResult.error()));
        return;
    }
    auto & fileTransferMetadata = fileTransferMetadataResult.value();
    if(fileTransferMetadata.fileIsFirmware)
    {
        auto partition = fw::GetPartition(fileTransferMetadata.destinationPartitionId);
        auto eraseResult = fw::Erase(partition.flashSector);
        if(eraseResult.has_error())
        {
            DEBUG_PRINT("Failed to erase partition '%s'\n",
                        ToCZString(fileTransferMetadata.destinationPartitionId));
            SendAndWait(
                FailedCompletionOfExecutionVerificationReport(requestId, eraseResult.error()));
            return;
        }
    }
    else if(fileTransferMetadata.destinationEntityId == cubeSatEntityId)
    {
        auto result = [&]() -> Result<void>
        {
            OUTCOME_TRY(auto file,
                        // NOLINTNEXTLINE(*signed-bitwise)
                        fs::Open(request.targetFilePath, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC));
            return file.Resize(request.fileSize);
        }();
        if(result.has_error())
        {
            DEBUG_PRINT("Failed to create and resize file '%s': %s\n",
                        request.targetFilePath.c_str(),
                        ToCZString(result.error()));
            SendAndWait(FailedCompletionOfExecutionVerificationReport(requestId, result.error()));
            return;
        }
    }
    // This wakes up the file transfer thread if it is waiting for new file transfer metadata
    fileTransferMetadataMailbox.Overwrite(fileTransferMetadata);
    DEBUG_PRINT("Successfully initiated copying file '%s' to '%s'\n",
                request.sourceFilePath.c_str(),
                request.targetFilePath.c_str());
    SendAndWait(SuccessfulCompletionOfExecutionVerificationReport(requestId));
    if(fileTransferMetadata.sourceEntityId == cubeSatEntityId)
    {
        SuspendUntilNewTelemetryRecordIsAvailable();
    }
}


template<auto parseFunction>
auto VerifyAndHandle(PerformAFunctionRequest const & request, RequestId const & requestId) -> void
{
    auto parseResult = parseFunction(request.dataField);
    if(parseResult.has_error())
    {
        DEBUG_PRINT("Failed acceptance of request: %s\n", ToCZString(parseResult.error()));
        SendAndWait(FailedAcceptanceVerificationReport(requestId, parseResult.error()));
        return;
    }
    DEBUG_PRINT("Successfully accepted request\n");
    SendAndWait(SuccessfulAcceptanceVerificationReport(requestId));
    Handle(parseResult.value(), requestId);
}


auto Handle(ReportHousekeepingParameterReportFunction const & function,
            [[maybe_unused]] RequestId const & requestId) -> void
{
    auto nTelemetryRecords = static_cast<std::uint16_t>(telemetryMemory.Size());
    auto iBegin = std::min<std::uint16_t>(function.firstReportIndex, nTelemetryRecords);
    auto iEnd = std::min<std::uint16_t>(function.lastReportIndex + 1U, nTelemetryRecords);
    if(iBegin == iEnd)
    {
        DEBUG_PRINT("No housekeeping parameter reports to send\n");
        return;
    }
    DEBUG_PRINT(
        "Sending housekeeping parameter reports %" PRIu16 " to %" PRIu16 "\n", iBegin, iEnd - 1U);
    SetTxDataLength(iEnd - iBegin);
    for(auto i = iBegin; i < iEnd; ++i)
    {
        SendAndContinue(HousekeepingParameterReport(telemetryMemory.Get(i)));
        if(telemetryRecordMailbox.IsFull())
        {
            break;
        }
    }
    FinalizeTransmission();
}


auto Handle(EnableFileTransferFunction const & function, RequestId const & requestId) -> void
{
    persistentVariables.Store<"fileTransferWindowEnd">(CurrentRodosTime()
                                                       + function.durationInS * s);
    DEBUG_PRINT("Enabled file transfers for the next %" PRIu16 " s\n", function.durationInS);
    SendAndWait(SuccessfulCompletionOfExecutionVerificationReport(requestId));
    ResumeFileTransferThread();
}


auto Handle(SynchronizeTimeFunction const & function, RequestId const & requestId) -> void
{
    UpdateRealTimeOffset(function.realTime);
    DEBUG_PRINT("Successfully synchronized time: current real time = %" PRIi32 "\n",
                value_of(CurrentRealTime()));
    SendAndWait(SuccessfulCompletionOfExecutionVerificationReport(requestId));
}


auto Handle(UpdateEduQueueFunction const & function, RequestId const & requestId) -> void
{
    persistentVariables.Store<"eduProgramQueueIndex">(eduProgramQueueIndexResetValue);
    edu::programQueue.Clear();
    for(auto && entry : function.queueEntries)
    {
        edu::programQueue.PushBack(entry);
    }
    DEBUG_PRINT("Updated EDU queue with %d entries\n",
                static_cast<int>(function.queueEntries.size()));
    SendAndWait(SuccessfulCompletionOfExecutionVerificationReport(requestId));
    ResumeEduProgramQueueThread();
}


auto Handle(SetActiveFirmwareFunction const & function, RequestId const & requestId) -> void
{
    persistentVariables.Store<"activeSecondaryFwPartitionId">(function.partitionId);
    DEBUG_PRINT("Set active firmware partition to %s\n", ToCZString(function.partitionId));
    SendAndWait(SuccessfulCompletionOfExecutionVerificationReport(requestId));
}


auto Handle(SetBackupFirmwareFunction const & function, RequestId const & requestId) -> void
{
    persistentVariables.Store<"backupSecondaryFwPartitionId">(function.partitionId);
    DEBUG_PRINT("Set backup firmware partition to %s\n", ToCZString(function.partitionId));
    SendAndWait(SuccessfulCompletionOfExecutionVerificationReport(requestId));
}


auto Handle(CheckFirmwareIntegrityFunction const & function, RequestId const & requestId) -> void
{
    auto result = [&]() -> Result<void>
    {
        auto partition = fw::GetPartition(function.partitionId);
        OUTCOME_TRY(fw::CheckFirmwareIntegrity(partition.startAddress));
        DEBUG_PRINT("Firmware in partition %s is intact\n", ToCZString(function.partitionId));
        DEBUG_PRINT("Successfully passed firmware integrity check for partition %s\n",
                    ToCZString(function.partitionId));
        SendAndWait(SuccessfulCompletionOfExecutionVerificationReport(requestId));
        return outcome_v2::success();
    }();
    if(result.has_error())
    {
        DEBUG_PRINT("Failed to check firmware integrity: %s\n", ToCZString(result.error()));
        SendAndWait(FailedCompletionOfExecutionVerificationReport(requestId, result.error()));
        return;
    }
}


auto ToRequestId(SpacePacketPrimaryHeader const & header) -> RequestId
{
    return RequestId{.packetVersionNumber = header.versionNumber,
                     .packetType = header.packetType,
                     .secondaryHeaderFlag = header.secondaryHeaderFlag,
                     .apid = header.apid,
                     .sequenceFlags = header.sequenceFlags,
                     .packetSequenceCount = header.packetSequenceCount};
}


auto GetValue(Parameter::Id parameterId) -> Parameter::Value
{
    switch(parameterId)
    {
        case Parameter::Id::rxDataRate:
            return rf::GetRxDataRate();
        case Parameter::Id::txDataRate:
            return rf::GetTxDataRate();
        case Parameter::Id::maxEduIdleDuration:
            return static_cast<std::uint32_t>(persistentVariables.Load<"maxEduIdleDuration">() / s);
        case Parameter::Id::newEduResultIsAvailable:
            return persistentVariables.Load<"newEduResultIsAvailable">() ? 1 : 0;
    }
    return 0;  // Should never be reached
}


auto Set(Parameter parameter) -> void
{
    switch(parameter.id)
    {
        case Parameter::Id::rxDataRate:
            rf::SetRxDataRate(parameter.value);
            rxDataRateTopic.publish(parameter.value);
            break;
        case Parameter::Id::txDataRate:
            rf::SetTxDataRate(parameter.value);
            txDataRateTopic.publish(parameter.value);
            break;
        case Parameter::Id::maxEduIdleDuration:
            persistentVariables.Store<"maxEduIdleDuration">(parameter.value * s);
            break;
        case Parameter::Id::newEduResultIsAvailable:
            persistentVariables.Store<"newEduResultIsAvailable">(parameter.value != 0U);
            break;
    }
}


// TODO: Refactor
// NOLINTNEXTLINE(*cognitive-complexity)
auto ValidateAndBuildFileTransferMetadata(CopyAFileRequest const & request)
    -> Result<FileTransferMetadata>
{
    static constexpr auto groundStationPathPrefix = "/gs/";
    auto sourceIsCubeSat = not request.sourceFilePath.starts_with(groundStationPathPrefix);
    auto targetIsCubeSat = not request.targetFilePath.starts_with(groundStationPathPrefix);
    if(sourceIsCubeSat == targetIsCubeSat)
    {
        DEBUG_PRINT("Both source and target paths are on %s\n",
                    sourceIsCubeSat ? "STS1" : "the ground");
        return ErrorCode::entityIdsAreIdentical;
    }
    auto fileTransferMetadata = FileTransferMetadata{
        .sourceEntityId = sourceIsCubeSat ? cubeSatEntityId : groundStationEntityId,
        .destinationEntityId = targetIsCubeSat ? cubeSatEntityId : groundStationEntityId,
        .fileIsFirmware = false,
        .sourcePath = request.sourceFilePath,
        .destinationPath = request.targetFilePath,
        .fileSize = request.fileSize};
    if(sourceIsCubeSat)
    {
        OUTCOME_TRY(auto file, fs::Open(request.sourceFilePath, LFS_O_RDONLY));
        OUTCOME_TRY(auto fileSize, file.Size());
        fileTransferMetadata.fileSize = fileSize;
    }
    else
    {
        static constexpr auto firmwarePathPrefix = "/firmware/";
        // TODO: Should we really be this strict?
        if(request.targetFilePath.starts_with(edu::programsDirectory))
        {
            // The + 1 is to account for the '/' after edu::programsDirectory
            auto filename = request.targetFilePath.substr(edu::programsDirectory.size() + 1);
            OUTCOME_TRY(edu::GetProgramId(filename));
        }
        else if(request.targetFilePath.starts_with(firmwarePathPrefix))
        {
            OUTCOME_TRY(auto partitionId, GetPartitionId(request.targetFilePath));
            fileTransferMetadata.destinationPartitionId = partitionId;
            fileTransferMetadata.fileIsFirmware = true;
        }
        else
        {
            return ErrorCode::invalidCubeSatFilePath;
        }
    }
    return fileTransferMetadata;
}


auto GetPartitionId(fs::Path const & filePath) -> Result<PartitionId>
{
    if(filePath == "/firmware/1")
    {
        return PartitionId::secondary1;
    }
    if(filePath == "/firmware/2")
    {
        return PartitionId::secondary2;
    }
    return ErrorCode::invalidFirmwarePath;
}


auto SetTxDataLength(std::uint16_t nFrames) -> void
{
    nFramesToSend = nFrames;
    nSentFrames = 0;
    rf::SetTxDataLength(std::min<std::uint16_t>(nFrames, maxNFramesToSendContinously)
                        * fullyEncodedFrameLength);
}


auto SendAndWait(Payload const & report) -> void
{
    PackageAndEncode(report);
    SendAndWait(tmBuffer);
}


auto SendAndContinue(Payload const & report) -> void
{
    PackageAndEncode(report);
    SendAndContinue(tmBuffer);
}


auto PackageAndEncode(Payload const & report) -> void
{
    tmFrame.StartNew(pusVcid);
    auto result = AddSpacePacketTo(&tmFrame.GetDataField(), normalApid, report);
    if(result.has_error())
    {
        DEBUG_PRINT("Failed to package report: %s\n", ToCZString(result.error()));
    }
    tmFrame.Finish();
    tm::Encode(tmBlock);
    // Write the attached sync marker to the beginning of the buffer every time because the constant
    // is stored in flash and therefore not corrupted as fast as the buffer in RAM.
    if constexpr(attachedSynchMarkerLength == attachedSynchMarker.size())
    {
        std::ranges::copy(attachedSynchMarker, tmBuffer.begin());
    }
}


auto SendAndWait(std::span<Byte const, channelAccessDataUnitLength> channelAccessDataUnit) -> void
{
    SuspendUntilEarliestTxTime();
    rf::SendAndWait(channelAccessDataUnit);
}


auto SendAndContinue(std::span<Byte const, channelAccessDataUnitLength> channelAccessDataUnit)
    -> void
{
    if(nSentFrames >= maxNFramesToSendContinously)
    {
        FinalizeTransmission();
        SetTxDataLength(nFramesToSend - nSentFrames);
    }
    SuspendUntilEarliestTxTime();
    rf::SendAndContinue(channelAccessDataUnit);
}


auto SuspendUntilEarliestTxTime() -> void
{
    auto earliestTxTime = lastRxTime + rxToTxSwitchDuration;
    if(earliestTxTime > CurrentRodosTime())
    {
        SuspendUntil(earliestTxTime);
    }
}


auto FinalizeTransmission() -> void
{
    // TX FIFO buffer size <= 128 B, slowest data rate = 1.2 kbps → max. send time = 128 B * 8 b/B /
    // 1200 b/s = 0.853 s → timeout = 1 s
    static constexpr auto timeout = 1 * s;
    rf::SuspendUntilDataSent(timeout);
    rf::EnterStandbyMode();
}
}
}
