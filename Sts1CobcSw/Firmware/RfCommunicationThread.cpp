#include <Sts1CobcSw/Firmware/RfCommunicationThread.hpp>

#include <Sts1CobcSw/ChannelCoding/ChannelCoding.hpp>
#include <Sts1CobcSw/Edu/ProgramQueue.hpp>
#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/FileSystem/DirectoryIterator.hpp>
#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/Firmware/FileTransferThread.hpp>
#include <Sts1CobcSw/Firmware/SpiStartupTestAndSupervisorThread.hpp>
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
#include <Sts1CobcSw/Vocabulary/MessageTypeIdFields.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>
#include <Sts1CobcSw/WatchdogTimers/WatchdogTimers.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <algorithm>
#include <array>
#include <cinttypes>  // IWYU pragma: keep
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
constexpr auto stackSize = 3000;
constexpr auto rxTimeout = 3 * s;
constexpr auto rxTimeoutAfterTelemetryRecord = 5 * s;
constexpr auto rxToTxSwitchDuration = 300 * ms;

auto tmBuffer = std::array<Byte, blockLength>{};
auto tcBuffer = std::array<Byte, blockLength>{};
auto tmFrame = tm::TransferFrame(std::span(tmBuffer).first<tm::transferFrameLength>());
auto lastRxTime = RodosTime(0);


auto Send(Payload const & report) -> void;
auto Send(std::span<Byte const, blockLength> encodedFrame) -> void;

auto SendCfdpFrames() -> void;
auto HandleReceivedData() -> void;
auto HandleCfdpFrame(tc::TransferFrame const & frame) -> void;
auto HandleRequestFrame(tc::TransferFrame const & frame) -> void;
auto ToRequestId(SpacePacketPrimaryHeader const & header) -> RequestId;
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

auto GetValue(Parameter::Id parameterId) -> Parameter::Value;
auto Set(Parameter parameter) -> void;


class RfCommunicationThread : public RODOS::StaticThread<stackSize>
{
public:
    RfCommunicationThread() : StaticThread("RfCommunicationThread", rfCommunicationThreadPriority)
    {}


private:
    auto run() -> void override
    {
        SuspendFor(totalStartupTestTimeout);  // Wait for the startup tests to complete
        DEBUG_PRINT("Starting RF communication thread\n");
        rdt::Initialize();
        while(true)
        {
            auto receiveResult = Result<void>(ErrorCode::timeout);
            auto newTelemetryRecordIsAvailable = telemetryRecordMailbox.IsFull();
            if(newTelemetryRecordIsAvailable)
            {
                auto getTelemetryRecordResult = telemetryRecordMailbox.Get();
                DEBUG_PRINT("Sending housekeeping parameter report\n");
                Send(HousekeepingParameterReport(getTelemetryRecordResult.value()));
                DEBUG_PRINT("Receiving for %" PRIi64 " s\n", rxTimeoutAfterTelemetryRecord / s);
                receiveResult = rf::Receive(tcBuffer, rxTimeoutAfterTelemetryRecord);
            }
            if(not newTelemetryRecordIsAvailable or receiveResult.has_error())
            {
                if(encodedCfdpFrameMailbox.IsFull())
                {
                    SendCfdpFrames();
                }
                else  // TODO: Maybe add if(not newTelemetryRecordIsAvailable) here
                {
                    DEBUG_PRINT("Receiving for %" PRIi64 " s\n", rxTimeout / s);
                    receiveResult = rf::Receive(tcBuffer, rxTimeout);
                }
            }
            if(receiveResult.has_value())
            {
                lastRxTime = CurrentRodosTime();
                HandleReceivedData();
                continue;
            }
            SuspendUntil(endOfTime);
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
auto Send(Payload const & report) -> void
{
    tmFrame.StartNew(pusVcid);
    // TODO: We know that the payload is neither empty nor too large but maybe we should
    // at least assert() that.
    (void)AddSpacePacketTo(&tmFrame.GetDataField(), normalApid, report);
    tmFrame.Finish();
    tm::Encode(tmBuffer);
    Send(tmBuffer);
}


auto Send(std::span<Byte const, blockLength> encodedFrame) -> void
{
    auto earliestTxTime = lastRxTime + rxToTxSwitchDuration;
    if(earliestTxTime > CurrentRodosTime())
    {
        SuspendUntil(earliestTxTime);
    }
    // TODO: Once the RF driver implements full error handling (retrying, reconfiguring,
    // and resetting). This should no longer return a Result<void>.
    (void)rf::SendAndWait(encodedFrame);
}


auto SendCfdpFrames() -> void
{
    DEBUG_PRINT("SendCfdpFrames()\n");
    // TODO: Implement this
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
    DEBUG_PRINT("HandleCfdpFrame()\n");
    // TODO: Implement this
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
        Send(FailedAcceptanceVerificationReport(requestId, parseAsRequestResult.error()));
        return;
    }
    auto const & request = parseAsRequestResult.value();
    persistentVariables.Store<"lastMessageTypeIdWasInvalid">(false);
    persistentVariables.Store<"lastMessageTypeId">(
        request.packetSecondaryHeader.messageTypeId.Value());
    Handle(request, requestId);
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
        Send(FailedAcceptanceVerificationReport(requestId, parseResult.error()));
        return;
    }
    // PerformAFunctionRequest has one more level of parsing to do before the successful acceptance
    // verification report can be sent
    if constexpr(not std::is_same_v<decltype(parseFunction),
                                    decltype(&ParseAsPerformAFunctionRequest)>)
    {
        DEBUG_PRINT("Successfully accepted request\n");
        Send(SuccessfulAcceptanceVerificationReport(requestId));
    }
    Handle(parseResult.value(), requestId);
}


auto Handle(LoadRawMemoryDataAreasRequest const & request, RequestId const & requestId) -> void
{
    static constexpr auto timeout = 100 * ms;
    fram::WriteTo(request.startAddress, request.data, timeout);
    DEBUG_PRINT("Successfully loaded raw memory data areas\n");
    Send(SuccessfulCompletionOfExecutionVerificationReport(requestId));
}


auto Handle(DumpRawMemoryDataRequest const & request, [[maybe_unused]] RequestId const & requestId)
    -> void
{
    auto dumpedData = etl::vector<Byte, maxDumpedDataLength>{};
    for(auto && dataArea : request.dataAreas)
    {
        dumpedData.uninitialized_resize(dataArea.length);
        static constexpr auto timeout = 100 * ms;
        fram::ReadFrom(dataArea.startAddress, std::span(dumpedData), timeout);
        DEBUG_PRINT("Sending dumped raw memory data report\n");
        Send(DumpedRawMemoryDataReport(1, dataArea.startAddress, dumpedData));
    }
}


auto Handle(PerformAFunctionRequest const & request, RequestId const & requestId) -> void
{
    switch(request.functionId)
    {
        case FunctionId::stopAntennaDeployment:
            persistentVariables.Store<"antennasShouldBeDeployed">(false);
            DEBUG_PRINT("Stopped antenna deployment\n");
            Send(SuccessfulCompletionOfExecutionVerificationReport(requestId));
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
            Send(SuccessfulCompletionOfExecutionVerificationReport(requestId));
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
    Send(ParameterValueReport(parameters));
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
    Send(ParameterValueReport(parameters));
}


auto Handle(DeleteAFileRequest const & request, RequestId const & requestId) -> void
{
    auto result = fs::Remove(request.filePath);
    if(result.has_error())
    {
        DEBUG_PRINT(
            "Failed to delete file %s: %s\n", request.filePath.c_str(), ToCZString(result.error()));
        Send(FailedCompletionOfExecutionVerificationReport(requestId, result.error()));
        return;
    }
    DEBUG_PRINT("Successfully deleted file %s\n", request.filePath.c_str());
    Send(SuccessfulCompletionOfExecutionVerificationReport(requestId));
}


auto Handle(ReportTheAttributesOfAFileRequest const & request, RequestId const & requestId) -> void
{
    auto result = [&]() -> Result<void>
    {
        OUTCOME_TRY(auto isLocked, fs::IsLocked(request.filePath));
        OUTCOME_TRY(auto fileSize, fs::FileSize(request.filePath));
        auto fileStatus = isLocked ? FileStatus::locked : FileStatus::unlocked;
        DEBUG_PRINT("Sending file attribute report for %s\n", request.filePath.c_str());
        Send(FileAttributeReport(request.filePath, fileSize, fileStatus));
        return outcome_v2::success();
    }();
    if(result.has_error())
    {
        DEBUG_PRINT("Failed to report attributes of file %s: %s\n",
                    request.filePath.c_str(),
                    ToCZString(result.error()));
        Send(FailedCompletionOfExecutionVerificationReport(requestId, result.error()));
    }
}


auto Handle(SummaryReportTheContentOfARepositoryRequest const & request,
            RequestId const & requestId) -> void
{
    auto result = [&]() -> Result<void>
    {
        OUTCOME_TRY(auto iterator, fs::MakeIterator(request.repositoryPath));
        auto nObjects = static_cast<std::uint8_t>(std::distance(iterator, iterator.end()));
        OUTCOME_TRY(iterator, fs::MakeIterator(request.repositoryPath));
        auto objects =
            etl::vector<FileSystemObject, RepositoryContentSummaryReport::maxNObjectsPerPacket>{};
        while(iterator != iterator.end())
        {
            OUTCOME_TRY(auto directoryInfo, *iterator);
            auto objectType = directoryInfo.type == fs::EntryType::file
                                ? FileSystemObject::Type::file
                                : FileSystemObject::Type::directory;
            objects.push_back({objectType, directoryInfo.name});
            ++iterator;
            if(objects.full() or iterator == iterator.end())
            {
                DEBUG_PRINT("Sending repository content summary report for %s\n",
                            request.repositoryPath.c_str());
                Send(RepositoryContentSummaryReport(request.repositoryPath, nObjects, objects));
                objects.clear();
            }
        }
        return outcome_v2::success();
    }();
    if(result.has_error())
    {
        DEBUG_PRINT("Failed to report content of repository %s: %s\n",
                    request.repositoryPath.c_str(),
                    ToCZString(result.error()));
        Send(FailedCompletionOfExecutionVerificationReport(requestId, result.error()));
    }
}


auto Handle(CopyAFileRequest const & request, RequestId const & requestId) -> void
{
    fileTransferInfoMailbox.Overwrite(FileTransferInfo{.sourcePath = request.sourceFilePath,
                                                       .destinationPath = request.targetFilePath});
    DEBUG_PRINT("Successfully initiated copying file %s to %s\n",
                request.sourceFilePath.c_str(),
                request.targetFilePath.c_str());
    Send(SuccessfulCompletionOfExecutionVerificationReport(requestId));
    ResumeFileTransferThread();
    SuspendUntil(endOfTime);
}


template<auto parseFunction>
auto VerifyAndHandle(PerformAFunctionRequest const & request, RequestId const & requestId) -> void
{
    auto parseResult = parseFunction(request.dataField);
    if(parseResult.has_error())
    {
        DEBUG_PRINT("Failed acceptance of request: %s\n", ToCZString(parseResult.error()));
        Send(FailedAcceptanceVerificationReport(requestId, parseResult.error()));
        return;
    }
    DEBUG_PRINT("Successfully accepted request\n");
    Send(SuccessfulAcceptanceVerificationReport(requestId));
    Handle(parseResult.value(), requestId);
}


auto Handle(ReportHousekeepingParameterReportFunction const & function,
            [[maybe_unused]] RequestId const & requestId) -> void
{
    auto nTelemetryRecords = static_cast<std::uint16_t>(telemetryMemory.Size());
    auto iMax = std::min<std::uint16_t>(nTelemetryRecords, function.lastReportIndex + 1U);
    DEBUG_PRINT("Sending housekeeping parameter report %" PRIu16 " to %" PRIu16 "\n",
                function.firstReportIndex,
                iMax - 1U);
    for(auto i = function.firstReportIndex; i < iMax; ++i)
    {
        Send(HousekeepingParameterReport(telemetryMemory.Get(i)));
    }
}


auto Handle(EnableFileTransferFunction const & function, RequestId const & requestId) -> void
{
    persistentVariables.Store<"fileTransferWindowEnd">(CurrentRodosTime()
                                                       + function.durationInS * s);
    DEBUG_PRINT("Enabled file transfers for the next %" PRIu16 " s\n", function.durationInS);
    Send(SuccessfulCompletionOfExecutionVerificationReport(requestId));
}


auto Handle(SynchronizeTimeFunction const & function, RequestId const & requestId) -> void
{
    UpdateRealTimeOffset(function.realTime);
    DEBUG_PRINT("Successfully synchronized time: current real time = %" PRIi32 "\n",
                value_of(CurrentRealTime()));
    Send(SuccessfulCompletionOfExecutionVerificationReport(requestId));
}


auto Handle(UpdateEduQueueFunction const & function, RequestId const & requestId) -> void
{
    edu::programQueue.Clear();
    for(auto && entry : function.queueEntries)
    {
        edu::programQueue.PushBack(entry);
    }
    DEBUG_PRINT("Updated EDU queue with %u entries\n", function.queueEntries.size());
    Send(SuccessfulCompletionOfExecutionVerificationReport(requestId));
}


auto Handle(SetActiveFirmwareFunction const & function, RequestId const & requestId) -> void
{
    persistentVariables.Store<"activeSecondaryFwPartition">(function.partitionId);
    DEBUG_PRINT("Set active firmware partition to %s\n", ToCZString(function.partitionId));
    Send(SuccessfulCompletionOfExecutionVerificationReport(requestId));
}


auto Handle(SetBackupFirmwareFunction const & function, RequestId const & requestId) -> void
{
    persistentVariables.Store<"backupSecondaryFwPartition">(function.partitionId);
    DEBUG_PRINT("Set backup firmware partition to %s\n", ToCZString(function.partitionId));
    Send(SuccessfulCompletionOfExecutionVerificationReport(requestId));
}


auto Handle(CheckFirmwareIntegrityFunction const & function, RequestId const & requestId) -> void
{
    auto result = [&]() -> Result<void>
    {
        OUTCOME_TRY(auto partition, fw::GetPartition(function.partitionId));
        OUTCOME_TRY(fw::CheckFirmwareIntegrity(partition.startAddress));
        DEBUG_PRINT("Firmware in partition %s is intact\n", ToCZString(function.partitionId));
        DEBUG_PRINT("Successfully passed firmware integrity check for partition %s\n",
                    ToCZString(function.partitionId));
        Send(SuccessfulCompletionOfExecutionVerificationReport(requestId));
        return outcome_v2::success();
    }();
    if(result.has_error())
    {
        DEBUG_PRINT("Failed to check firmware integrity: %s\n", ToCZString(result.error()));
        Send(FailedCompletionOfExecutionVerificationReport(requestId, result.error()));
        return;
    }
}


auto GetValue(Parameter::Id parameterId) -> Parameter::Value
{
    switch(parameterId)
    {
        case Parameter::Id::rxDataRate:
            return rf::GetRxDataRate();
        case Parameter::Id::txDataRate:
            return rf::GetTxDataRate();
        case Parameter::Id::realTimeOffsetCorrection:
            return static_cast<std::uint32_t>(persistentVariables.Load<"realTimeOffsetCorrection">()
                                              / s);
        case Parameter::Id::eduStartDelayLimit:
            return static_cast<std::uint32_t>(persistentVariables.Load<"eduStartDelayLimit">() / s);
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
            break;
        case Parameter::Id::txDataRate:
            rf::SetTxDataRate(parameter.value);
            break;
        case Parameter::Id::realTimeOffsetCorrection:
            persistentVariables.Store<"realTimeOffsetCorrection">(parameter.value * s);
            break;
        case Parameter::Id::eduStartDelayLimit:
            persistentVariables.Store<"eduStartDelayLimit">(parameter.value * s);
            break;
        case Parameter::Id::newEduResultIsAvailable:
            persistentVariables.Store<"newEduResultIsAvailable">(parameter.value != 0U);
            break;
    }
}
}
}
