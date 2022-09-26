#include <rodos_no_using_namespace.h>

#include <tuple>

namespace sts1cobcsw
{
// A queue consists of :
// Program ID 	: 2 bytes, according to EDU PDD 6.1.1
// Queue ID 	: 2 bytes, according to EDU PDD 6.1.2
// Start Time 	: 4 bytes, EPOCH time
// Timeout 		: 2 bytes, according to EDU PDD 6.1.2
using QueueEntry = std::tuple<uint16_t, uint16_t, uint32_t, uint16_t>;

// Define the maximum size of our queue
constexpr auto eduProgramQueueSize = 20;

// Use a commbuffer to share the edu program queue among differents threads
// such as edu listener or initialize
RODOS::CommBuffer<std::array<QueueEntry, eduProgramQueueSize>> eduQueueBufffer;

class EduProgramQueue : public RODOS::StaticThread<>
{
    void run() override
    {
        std::array<QueueEntry, eduProgramQueueSize> eduQueue;
	
		// NOLINTNEXTLINE
		QueueEntry prog1 = std::make_tuple(1,1,100,30);
		eduQueue.at(1) = prog1;
    }
};

auto const eduProgramQueue = EduProgramQueue();
}
