#pragma once

#include <rodos_no_using_namespace.h>

#include <tuple>


namespace sts1cobcsw
{
// A queue consists of :
// Program ID 	: 2 bytes, according to EDU PDD 6.1.1
// Queue ID 	: 2 bytes, according to EDU PDD 6.1.2
// Start Time 	: 4 bytes, EPOCH time
// Timeout 		: 2 bytes, according to EDU PDD 6.1.2
constexpr auto eduProgramQueueSize = 20;


using QueueEntry = std::tuple<uint16_t, uint16_t, uint32_t, uint16_t>;
}
