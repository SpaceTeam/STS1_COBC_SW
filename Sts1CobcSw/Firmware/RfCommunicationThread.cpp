#include <Sts1CobcSw/Firmware/RfCommunicationThread.hpp>

#include <Sts1CobcSw/ChannelCoding/ChannelCoding.hpp>
#include <Sts1CobcSw/Firmware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/Mailbox/Mailbox.hpp>
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

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <array>
#include <span>


namespace sts1cobcsw
{
namespace
{
constexpr auto stackSize = 1000;

auto tmBuffer = std::array<Byte, blockLength>{};
auto tmFrame = tm::TransferFrame(std::span(tmBuffer).first<tm::transferFrameLength>());


auto Send(Payload const & report) -> void;


class RfCommunicationThread : public RODOS::StaticThread<stackSize>
{
public:
    RfCommunicationThread() : StaticThread("RfCommunicationThread", rfCommunicationThreadPriority)
    {}


private:
    auto run() -> void override
    {
        SuspendFor(totalStartupTestTimeout);  // Wait for the startup tests to complete
        while(true)
        {
            auto getTelemetryRecordResult = telemetryRecordMailbox.Get();
            if(getTelemetryRecordResult.has_value())
            {
                Send(HousekeepingParameterReport(getTelemetryRecordResult.value()));
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
}
}
