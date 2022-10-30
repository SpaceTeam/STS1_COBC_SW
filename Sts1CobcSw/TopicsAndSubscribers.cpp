#include <Sts1CobcSw/TopicsAndSubscribers.hpp>


namespace sts1cobcsw
{
// Topics and subscribers must be defined in the same file to prevent a static initialization order
// fiasco
RODOS::Topic<bool> eduIsAliveTopic(-1, "eduIsAliveTopic");
RODOS::CommBuffer<bool> eduIsAliveBuffer{};
RODOS::Subscriber eduIsAliveSubscriber(eduIsAliveTopic, eduIsAliveBuffer, "eduIsAliveSubscriber");

RODOS::Topic<std::int64_t> nextProgramStartDelayTopic(-1, "nextProgramStartDelayTopic");
RODOS::CommBuffer<std::int64_t> nextProgramStartDelayBuffer{};
RODOS::Subscriber nextProgramStartDelaySubscriber(nextProgramStartDelayTopic,
                                                  nextProgramStartDelayBuffer,
                                                  "nextProgramStartDelaySubscriber");
}
