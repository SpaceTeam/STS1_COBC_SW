#include <Sts1CobcSw/EduProgramQueue.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
std::uint16_t queueIndex = 0U;
etl::vector<EduQueueEntry, eduProgramQueueSize> eduProgramQueue{};
RODOS::RingBuffer<StatusHistoryEntry, statusHistorySize> statusHistory{};


// TODO: Remove useless wrapper
void AddQueueEntry(const EduQueueEntry & eduEntry)
{
    eduProgramQueue.push_back(eduEntry);
}


void ResetQueueIndex()
{
    queueIndex = 0U;
    RODOS::PRINTF("QueueIndex reset. Current size of edu program queue is : %d\n",
                  eduProgramQueue.size());
}


// TODO: Remove useless wrapper
void EmptyEduProgramQueue()
{
    eduProgramQueue.clear();
}
}
