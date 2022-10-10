#pragma once

#include <rodos_no_using_namespace.h>

#include <etl/string.h>

#include <tuple>


namespace sts1cobcsw
{
constexpr auto eduProgramQueueSize = 20;
constexpr auto statusHistorySize = 20;


// A queue consists of :
// Program ID 	: 2 bytes, according to EDU PDD 6.1.1
// Queue ID 	: 2 bytes, according to EDU PDD 6.1.2
// Start Time 	: 4 bytes, EPOCH time
// Timeout 		: 2 bytes, according to EDU PDD 6.1.2
// That adds up to 10 bytes in total
using QueueEntry = std::tuple<uint16_t, uint16_t, uint32_t, uint16_t>;

// A stus and history entry consists of :
// Progam ID 	: 2 bytes
// Queue ID		: 2 bytes
// Status		: A string ? with max 2 characters
constexpr auto statusMaxSize = 2;
using StatusHistoryEntry = std::tuple<uint16_t, uint16_t, etl::string<statusMaxSize>>;

class TimeEventTest : public RODOS::TimeEvent
{
	public:
	void handle() override;
};



}
