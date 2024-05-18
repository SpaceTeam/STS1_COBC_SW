#pragma once


namespace sts1cobcsw
{
inline constexpr auto commandParserThreadPriority = 100;
inline constexpr auto eduListenerThreadPriority = 100;
inline constexpr auto framEpsStartupTestThreadPriority = 101;
inline constexpr auto flashStartupTestThreadPriority = 102;
inline constexpr auto rfStartupTestThreadPriority = 103;
inline constexpr auto eduProgramQueueThreadPriority = 300;
inline constexpr auto eduCommunicationErrorThreadPriority = 400;
inline constexpr auto eduPowerManagementThreadPriority = 500;
inline constexpr auto eduHeartbeatThreadPriority = 600;
}
