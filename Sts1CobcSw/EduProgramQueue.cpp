#include <rodos_no_using_namespace.h>

#include <tuple>

#include <Sts1CobcSw/EduProgramQueue.hpp>

namespace sts1cobcsw
{

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
