#pragma once


#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
// Topics and subscribers must be defined in the same file to prevent a static initialization order
// fiasco
extern RODOS::Topic<bool> eduIsAliveTopic;
extern RODOS::CommBuffer<bool> eduIsAliveBufferForListener;
extern RODOS::CommBuffer<bool> eduIsAliveBufferForPowerManagement;
extern RODOS::CommBuffer<bool> eduIsAliveBufferForCommunicationError;
extern RODOS::CommBuffer<bool> eduIsAliveBufferForTelemetry;
extern RODOS::Topic<Duration> nextProgramStartDelayTopic;
extern RODOS::CommBuffer<Duration> nextProgramStartDelayBuffer;
}
