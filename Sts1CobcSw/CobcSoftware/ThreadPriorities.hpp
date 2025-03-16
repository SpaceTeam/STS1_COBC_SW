#pragma once


#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
inline constexpr auto framEpsStartupTestThreadPriority = 97;
inline constexpr auto flashStartupTestThreadPriority = 98;
inline constexpr auto rfStartupTestThreadPriority = 99;
inline constexpr auto commandParserThreadPriority = 100;
inline constexpr auto eduListenerThreadPriority = 100;
inline constexpr auto eduProgramQueueThreadPriority = 300;
inline constexpr auto eduCommunicationErrorThreadPriority = 400;
inline constexpr auto eduPowerManagementThreadPriority = 500;
inline constexpr auto eduHeartbeatThreadPriority = 600;
inline constexpr auto spiStartupTestAndSupervisorThreadPriority = MAX_THREAD_PRIORITY;
inline constexpr auto telemetryThreadPriority = 200;        // TODO: Find a better value
inline constexpr auto rfCommunicationThreadPriority = 200;  // TODO: Find a better value
}
