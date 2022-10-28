#pragma once


#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
// Topics and subscribers must be defined in the same file to prevent a static initialization order
// fiasco
extern RODOS::Topic<bool> eduIsAliveTopic;
extern RODOS::CommBuffer<bool> eduIsAliveBuffer;
extern RODOS::Topic<int64_t> nextProgramStartDelayTopic;
extern RODOS::CommBuffer<int64_t> nextProgramStartDelayBuffer;
}
