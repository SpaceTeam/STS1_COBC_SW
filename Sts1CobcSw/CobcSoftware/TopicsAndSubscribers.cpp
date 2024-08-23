#include <Sts1CobcSw/CobcSoftware/TopicsAndSubscribers.hpp>


namespace sts1cobcsw
{
// Topics and subscribers must be defined in the same file to prevent a static initialization order
// fiasco
RODOS::Topic<bool> eduIsAliveTopic(-1, "eduIsAliveTopic");
RODOS::CommBuffer<bool> eduIsAliveBufferForPowerManagement{};
RODOS::CommBuffer<bool> eduIsAliveBufferForCommunicationError{};
RODOS::CommBuffer<bool> eduIsAliveBufferForListener{};

RODOS::Subscriber eduIsAliveSubscriberForPowerManagement(eduIsAliveTopic,
                                                         eduIsAliveBufferForPowerManagement,
                                                         "eduIsAliveSubscriber");
RODOS::Subscriber eduIsAliveSubscriberForCommunicationError(eduIsAliveTopic,
                                                            eduIsAliveBufferForCommunicationError,
                                                            "eduIsAliveSubscriber");
RODOS::Subscriber eduIsAliveSubscriberForListener(eduIsAliveTopic,
                                                  eduIsAliveBufferForListener,
                                                  "eduIsAliveSubscriber");

RODOS::Topic<Duration> nextProgramStartDelayTopic(-1, "nextProgramStartDelayTopic");
RODOS::CommBuffer<Duration> nextProgramStartDelayBuffer{};
RODOS::Subscriber nextProgramStartDelaySubscriber(nextProgramStartDelayTopic,
                                                  nextProgramStartDelayBuffer,
                                                  "nextProgramStartDelaySubscriber");
}
