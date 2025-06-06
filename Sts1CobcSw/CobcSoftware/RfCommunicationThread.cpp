#include <Sts1CobcSw/CobcSoftware/RfCommunicationThread.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/CobcSoftware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Rf/Rf.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/MessageTypeIdFields.hpp>
#include <Sts1CobcSw/RfProtocols/Reports.hpp>
#include <Sts1CobcSw/RfProtocols/SpacePacket.hpp>
#include <Sts1CobcSw/RfProtocols/TcTransferFrame.hpp>
#include <Sts1CobcSw/RfProtocols/TmTransferFrame.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <misc-rodos-funcs.h>

#include <rodos_no_using_namespace.h>

#include <array>
#include <span>


namespace sts1cobcsw
{
// TODO: Get a better estimation for the required stack size.
constexpr auto stackSize = 1000;

constexpr auto rxTimeoutAfterTmSend = 5 * s;

static auto HandleReceivedData(std::span<Byte> receivedData) -> void
{
    // TODO: Feed reset dog, set nResetsSinceRf to 0
    persistentVariables.template Store<"nResetsSinceRf">(0);

    // TODO: in-place decode, tracked in MR#385
    // rf::decode(&receiveddata)

    auto parseFrameResult = tc::ParseAsTransferFrame(receivedData.first<tc::transferFrameLength>());
    if(parseFrameResult.has_error())
    {
        return;
    }
    auto & receivedFrame = parseFrameResult.value();

    auto vcid = receivedFrame.primaryHeader.vcid;
    auto dataIsPartOfFileTransfer = (vcid == cfdpVcid) ? true : false;

    if(dataIsPartOfFileTransfer)
    {
        // ...
    }
    else
    {
        // requests not  implemented yet
        // Signature will be something like : 
        // rf::ParseAsTransferFrame(std::span<Byte const,  transferFrameLength> buffer) -> Result<TransferFrame>
    }
}

auto SendTmRecord(TelemetryRecord const & telemetryRecord) -> void
{
    // TODO: RS coding, tracked in MR#385
    auto housekeepingBuffer = std::array<Byte, tm::transferFrameLength>{};
    auto housekeepingParameterReport = HousekeepingParameterReport(telemetryRecord);

    auto frame = tm::TransferFrame(Span(&housekeepingBuffer));
    frame.StartNew(pusVcid);
    // TODO: Handle error when packaging
    auto addSpacePacketResult =
        AddSpacePacketTo(&frame.GetDataField(), normalApid, housekeepingParameterReport);
    frame.Finish();

    // TODO: Error handling
    // If an RF function fails after initialization, we should retry the function.
    // If it still fails, we reinitialize the RF module and try the function again.
    auto sendResult = rf::SendAndWait(Span(housekeepingBuffer));
    if(sendResult.has_error())
    {
        sendResult = rf::SendAndWait(Span(housekeepingBuffer));
        if(sendResult.has_error())
        {
            rf::Initialize(rf::TxType::packet);
            rf::EnableTx();
            sendResult = rf::SendAndWait(Span(housekeepingBuffer));

            if(sendResult.has_error())
            {
                RODOS::hwResetAndReboot();
            }
        }
    }


}


class RfCommunicationThread : public RODOS::StaticThread<stackSize>
{
public:
    RfCommunicationThread() : StaticThread("RfCommunicationThread", rfCommunicationThreadPriority)
    {
    }


private:
    void init() override
    {
        rf::Initialize(rf::TxType::packet);
    }


    void run() override
    {
        // Check if new telemetry record is available
        TelemetryRecord telemetryRecord{};
        auto newTmRecordIsAvailable = telemetryBuffer.getOnlyIfNewData(telemetryRecord);

        if(newTmRecordIsAvailable)
        {
            SendTmRecord(telemetryRecord);

            // TODO: use global buffer
            auto receivedData = std::array<Byte, 383>{};  // NOLINT(*magic-numbers)
            auto receiveResult = rf::Receive(Span(&receivedData), rxTimeoutAfterTmSend);

            if(not receiveResult.has_error())
            {
                HandleReceivedData(Span(&receivedData));
            } else {}
        }
        else
        {
            // TODO: implement the cfdp part
            auto shouldSendCfdp = false;
            if(shouldSendCfdp)
            {
                // ...
            }
            else
            {
                // TODO: use global buffer
                auto receivedData = std::array<Byte, 383>{};  // NOLINT(*magic-numbers)
                auto receiveResult = rf::Receive(Span(&receivedData), 3 * s);
                if(not receiveResult.has_error())
                {
                    HandleReceivedData(Span(&receivedData));
                }
            }
        }
        SuspendUntil(endOfTime);
    }
} rfCommunicationThread;


auto ResumeRfCommunicationThread() -> void
{
    rfCommunicationThread.resume();
}
}
