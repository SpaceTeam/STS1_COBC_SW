#include <Sts1CobcSw/CobcSoftware/TopicsAndSubscribers.hpp>


namespace sts1cobcsw
{
// Topics and subscribers must be defined in the same file to prevent a static initialization order
// fiasco
RODOS::Topic<bool> eduIsAliveTopic(-1, "eduIsAliveTopic");
RODOS::CommBuffer<bool> eduIsAliveBufferForPowerManagement{};
RODOS::CommBuffer<bool> eduIsAliveBufferForCommunicationError{};
RODOS::CommBuffer<bool> eduIsAliveBufferForListener{};
RODOS::CommBuffer<bool> eduIsAliveBufferForTelemetry{};
namespace
{
RODOS::Subscriber eduIsAliveSubscriberForPowerManagement(eduIsAliveTopic,
                                                         eduIsAliveBufferForPowerManagement,
                                                         "eduIsAliveSubscriber");
RODOS::Subscriber eduIsAliveSubscriberForCommunicationError(eduIsAliveTopic,
                                                            eduIsAliveBufferForCommunicationError,
                                                            "eduIsAliveSubscriber");
RODOS::Subscriber eduIsAliveSubscriberForListener(eduIsAliveTopic,
                                                  eduIsAliveBufferForListener,
                                                  "eduIsAliveSubscriber");
RODOS::Subscriber eduIsAliveSubscriberForTelemetry(eduIsAliveTopic,
                                                   eduIsAliveBufferForListener,
                                                   "eduIsAliveSubscriber");
}

RODOS::Topic<Duration> nextProgramStartDelayTopic(-1, "nextProgramStartDelayTopic");
RODOS::CommBuffer<Duration> nextProgramStartDelayBuffer{};
namespace
{
RODOS::Subscriber nextProgramStartDelaySubscriber(nextProgramStartDelayTopic,
                                                  nextProgramStartDelayBuffer,
                                                  "nextProgramStartDelaySubscriber");
}

RODOS::Topic<ProgramId> programIdOfCurrentEduProgramQueueEntryTopic(
    -1, "programIdOfCurrentEduProgramQueueEntryTopic");
RODOS::CommBuffer<ProgramId> programIdOfCurrentEduProgramQueueEntryBuffer{};
namespace
{
RODOS::Subscriber programIdOfCurrentEduProgramQueueEntrySubscriber(
    programIdOfCurrentEduProgramQueueEntryTopic,
    programIdOfCurrentEduProgramQueueEntryBuffer,
    "programIdOfCurrentEduProgramQueueEntrySubscriber");
}

RODOS::Topic<std::int32_t> rxBaudRateTopic(-1, "rxBaudRateTopic");
RODOS::CommBuffer<std::int32_t> rxBaudRateBuffer{};
namespace
{
RODOS::Subscriber rxBaudRateSubscriber(rxBaudRateTopic, rxBaudRateBuffer, "rxBaudRateSubscriber");
}

RODOS::Topic<std::int32_t> txBaudRateTopic(-1, "txBaudRateTopic");
RODOS::CommBuffer<std::int32_t> txBaudRateBuffer{};
namespace
{
RODOS::Subscriber txBaudRateSubscriber(txBaudRateTopic, txBaudRateBuffer, "txBaudRateSubscriber");
}

RODOS::Topic<TelemetryRecord> telemetryTopic(-1, "telemetryTopic");
RODOS::CommBuffer<TelemetryRecord> telemetryBuffer{};
namespace
{
RODOS::Subscriber telemetrySubscriber(telemetryTopic, telemetryBuffer, "telemetrySubscriber");
}
}
