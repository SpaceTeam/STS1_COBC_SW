#include <Sts1CobcSw/EduProgramQueue.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
std::uint16_t queueIndex = 0U;
etl::vector<EduQueueEntry, eduProgramQueueSize> eduProgramQueue{};
RODOS::RingBuffer<StatusHistoryEntry, statusHistorySize> statusHistory{};
}
