#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>


namespace sts1cobcsw
{
// Topics and subscribers must be defined in the same file to prevent a static initialization order
// fiasco
namespace
{
RODOS::Subscriber eduIsAliveSubscriberForCommunicationError(eduIsAliveTopic,
                                                            eduIsAliveBufferForCommunicationError,
                                                            "eduIsAliveSubscriber");
RODOS::Subscriber eduIsAliveSubscriberForListener(eduIsAliveTopic,
                                                  eduIsAliveBufferForListener,
                                                  "eduIsAliveSubscriber");
RODOS::Subscriber eduIsAliveSubscriberForPowerManagement(eduIsAliveTopic,
                                                         eduIsAliveBufferForPowerManagement,
                                                         "eduIsAliveSubscriber");
RODOS::Subscriber eduIsAliveSubscriberForProgramQueue(eduIsAliveTopic,
                                                      eduIsAliveBufferForProgramQueue,
                                                      "eduIsAliveSubscriber");
RODOS::Subscriber eduIsAliveSubscriberForTelemetry(eduIsAliveTopic,
                                                   eduIsAliveBufferForListener,
                                                   "eduIsAliveSubscriber");
RODOS::Subscriber nextEduProgramStartTimeSubscriber(nextEduProgramStartTimeTopic,
                                                    nextEduProgramStartTimeBuffer,
                                                    "nextProgramStartTimeSubscriber");
RODOS::Subscriber programIdOfCurrentEduProgramQueueEntrySubscriber(
    programIdOfCurrentEduProgramQueueEntryTopic,
    programIdOfCurrentEduProgramQueueEntryBuffer,
    "programIdOfCurrentEduProgramQueueEntrySubscriber");
RODOS::Subscriber rxDataRateSubscriber(rxDataRateTopic, rxDataRateBuffer, "rxDataRateSubscriber");
RODOS::Subscriber txDataRateSubscriber(txDataRateTopic, txDataRateBuffer, "txDataRateSubscriber");
}
}
