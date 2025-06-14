#include <Sts1CobcSw/Firmware/RfCommunicationThread.hpp>

#include <Sts1CobcSw/ChannelCoding/ChannelCoding.hpp>
#include <Sts1CobcSw/Firmware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Mailbox/Mailbox.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
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
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>
#include <Sts1CobcSw/Vocabulary/MessageTypeIdFields.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>
#include <Sts1CobcSw/WatchdogTimers/WatchdogTimers.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <array>
#include <span>
#include <utility>


namespace sts1cobcsw
{
namespace
{
constexpr auto stackSize = 1000;
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
auto Handle(Request const & request, RequestId const & requestId) -> void;

auto ToRequestId(SpacePacketPrimaryHeader const & header) -> RequestId;


class RfCommunicationThread : public RODOS::StaticThread<stackSize>
{
public:
    RfCommunicationThread() : StaticThread("RfCommunicationThread", rfCommunicationThreadPriority)
    {}


private:
    auto run() -> void override
    {
        SuspendFor(totalStartupTestTimeout);  // Wait for the startup tests to complete
        rdt::Initialize();
        while(true)
        {
            auto receiveResult = Result<void>(ErrorCode::timeout);
            auto newTelemetryRecordIsAvailable = telemetryRecordMailbox.IsFull();
            if(newTelemetryRecordIsAvailable)
            {
                auto getTelemetryRecordResult = telemetryRecordMailbox.Get();
                Send(HousekeepingParameterReport(getTelemetryRecordResult.value()));
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
        persistentVariables.Increment<"nBadTransferFrames">();
    }
}


auto HandleCfdpFrame(tc::TransferFrame const & frame) -> void
{
    // TODO: Implement this
}


auto HandleRequestFrame(tc::TransferFrame const & frame) -> void
{
    auto parseAsSpacePacketResult = ParseAsSpacePacket(frame.dataField);
    if(parseAsSpacePacketResult.has_error())
    {
        return;
    }
    auto const & spacePacket = parseAsSpacePacketResult.value();
    auto requestId = ToRequestId(spacePacket.primaryHeader);
    auto parseAsRequestResult = ParseAsRequest(spacePacket.dataField);
    if(parseAsRequestResult.has_error())
    {
        persistentVariables.Store<"lastMessageTypeIdWasInvalid">(
            parseAsRequestResult.error() == ErrorCode::invalidMessageTypeId);
        Send(FailedVerificationReport<VerificationStage::acceptance>(requestId,
                                                                     parseAsRequestResult.error()));
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
    // TODO: Implement this
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
}
}
