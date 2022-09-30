#pragma once

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
namespace ts = type_safe;


inline RODOS::Topic<bool> eduIsAliveTopic(-1, "eduHeartBeatsTopic");
inline RODOS::Topic<ts::int32_t> temperatureTopic(-1, "temperatureTopic");
inline RODOS::Topic<ts::int32_t> accelerationXTopic(-1, "accelerationXTopic");
inline RODOS::Topic<ts::int32_t> accelerationYTopic(-1, "accelerationYTopic");
inline RODOS::Topic<ts::int32_t> accelerationZTopic(-1, "accelerationZTopic");
inline RODOS::Topic<ts::int32_t> brightnessTopic(-1, "brightnessTopic");
}
