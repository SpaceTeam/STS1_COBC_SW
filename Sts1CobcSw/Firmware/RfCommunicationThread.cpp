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
#include <Sts1CobcSw/RfProtocols/Payload.hpp>
#include <Sts1CobcSw/RfProtocols/Reports.hpp>
#include <Sts1CobcSw/RfProtocols/SpacePacket.hpp>
#include <Sts1CobcSw/RfProtocols/TmTransferFrame.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

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

auto tmBuffer = std::array<Byte, blockLength>{};
auto tcBuffer = std::array<Byte, blockLength>{};
auto tmFrame = tm::TransferFrame(std::span(tmBuffer).first<tm::transferFrameLength>());


auto Send(Payload const & report) -> void;
auto SendCfdpFrames() -> void;
// Handle received data and return true if the thread should continue the loop without suspending
[[nodiscard]] auto HandleReceivedData() -> bool;


class RfCommunicationThread : public RODOS::StaticThread<stackSize>
{
public:
    RfCommunicationThread() : StaticThread("RfCommunicationThread", rfCommunicationThreadPriority)
    {}


private:
    auto run() -> void override
    {
        SuspendFor(totalStartupTestTimeout);  // Wait for the startup tests to complete
        // TODO: rdt::Initialize();
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
                auto threadShouldNotSuspend = HandleReceivedData();
                if(threadShouldNotSuspend)
                {
                    continue;
                }
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
    // TODO: Once the RF driver implements full error handling (retrying, reconfiguring,
    // and resetting). This will no longer return a Result<void>.
    (void)rf::SendAndWait(tmBuffer);
}


auto SendCfdpFrames() -> void
{
    // TODO: Implement it
}


auto HandleReceivedData() -> bool
{
    // TODO: rdt::Feed();
    persistentVariables.Store<"nResetsSinceRf">(0);
    auto threadShouldNotSuspend = true;
    auto decodeResult = tc::Decode(tcBuffer);
    // TODO: Continue here with the implementation
    if(decodeResult.has_error())
    {
        persistentVariables.Increment<"nUncorrectableUplinkErrors">();
        persistentVariables.Increment<"nBadTransferFrames">();
    }
    return threadShouldNotSuspend;
}
}
}
