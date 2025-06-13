#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>


namespace sts1cobcsw
{
// Topics and subscribers must be defined in the same file to prevent a static initialization order
// fiasco
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
RODOS::Subscriber nextProgramStartDelaySubscriber(nextProgramStartDelayTopic,
                                                  nextProgramStartDelayBuffer,
                                                  "nextProgramStartDelaySubscriber");
RODOS::Subscriber programIdOfCurrentEduProgramQueueEntrySubscriber(
    programIdOfCurrentEduProgramQueueEntryTopic,
    programIdOfCurrentEduProgramQueueEntryBuffer,
    "programIdOfCurrentEduProgramQueueEntrySubscriber");
RODOS::Subscriber rxBaudRateSubscriber(rxBaudRateTopic, rxBaudRateBuffer, "rxBaudRateSubscriber");
RODOS::Subscriber txBaudRateSubscriber(txBaudRateTopic, txBaudRateBuffer, "txBaudRateSubscriber");
}
}
