#include <Sts1CobcSw/EduProgramQueue.hpp>

namespace sts1cobcsw {


void AddQueueEntry(const QueueEntry & eduEntry)
{
    eduProgramQueue.push_back(eduEntry);
}


void ResetQueueIndex()
{
    queueIndex = 0U;
    RODOS::PRINTF("QueueIndex reset. Current size of edu program queue is : %d\n", eduProgramQueue.size());
}


void EmptyEduProgramQueue()
{
    eduProgramQueue.clear();
}

}
