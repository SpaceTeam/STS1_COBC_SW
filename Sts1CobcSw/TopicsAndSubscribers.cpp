#include <Sts1CobcSw/TopicsAndSubscribers.hpp>


namespace sts1cobcsw
{
// Topics and subscribers must be defined in the same file to prevent a static initialization order
// fiasco
RODOS::Topic<bool> eduIsAliveTopic(-1, "eduIsAliveTopic");
RODOS::CommBuffer<bool> eduIsAliveBuffer{};
RODOS::Subscriber eduIsAliveSubscriber(eduIsAliveTopic, eduIsAliveBuffer, "eduIsAliveSubscriber");
}
