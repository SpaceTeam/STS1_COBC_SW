#pragma once


#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw
{
// Topics and subscribers must be defined in the same file to prevent a static initialization order
// fiasco
extern RODOS::Topic<bool> eduIsAliveTopic;
extern RODOS::CommBuffer<bool> eduIsAliveBufferForListener;
extern RODOS::CommBuffer<bool> eduIsAliveBufferForPowerManagement;
extern RODOS::CommBuffer<bool> eduIsAliveBufferForCommunicationError;
extern RODOS::Topic<std::int64_t> nextProgramStartDelayTopic;
extern RODOS::CommBuffer<std::int64_t> nextProgramStartDelayBuffer;
}
