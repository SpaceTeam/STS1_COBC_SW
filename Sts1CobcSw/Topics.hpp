#pragma once

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
namespace ts = type_safe;
using RODOS::Topic;


inline Topic<bool> eduIsAliveTopic(-1, "eduHeartBeatsTopic");
inline Topic<ts::int32_t> temperatureTopic(-1, "temperatureTopic");
inline Topic<ts::int32_t> accelerationXTopic(-1, "accelerationXTopic");
inline Topic<ts::int32_t> accelerationYTopic(-1, "accelerationYTopic");
inline Topic<ts::int32_t> accelerationZTopic(-1, "accelerationZTopic");
inline Topic<ts::int32_t> brightnessTopic(-1, "brightnessTopic");
}
