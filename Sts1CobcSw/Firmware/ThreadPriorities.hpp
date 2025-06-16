#pragma once


#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
inline constexpr auto framEpsStartupTestThreadPriority = 97;
inline constexpr auto flashStartupTestThreadPriority = 98;
inline constexpr auto rfStartupTestThreadPriority = 99;
inline constexpr auto eduListenerThreadPriority = 210;
inline constexpr auto eduProgramQueueThreadPriority = 230;
inline constexpr auto eduCommunicationErrorThreadPriority = 240;
inline constexpr auto eduPowerManagementThreadPriority = 250;
inline constexpr auto eduHeartbeatThreadPriority = 260;
inline constexpr auto spiStartupTestAndSupervisorThreadPriority = MAX_THREAD_PRIORITY;
inline constexpr auto telemetryThreadPriority = 910;        // TODO: Find a better value
inline constexpr auto rfCommunicationThreadPriority = 900;  // TODO: Find a better value
}
