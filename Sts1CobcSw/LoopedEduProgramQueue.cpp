#include <Sts1CobcSw/EduProgramQueue.hpp>
#include <Sts1CobcSw/LoopedEduProgramQueue.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
constexpr auto initialProgramDelay = 30;         // s; should be > eduBootTime
constexpr auto eduProgramQueuePeriod = 10 * 60;  // s

constexpr auto sensorProgramId = 1;
constexpr auto sensorProgramStartTimeOffset = 0;  // s
constexpr auto sensorProgramTimeout = 1 * 60;     // s

constexpr auto fotoProgramId = 2;
constexpr auto fotoProgramStartTimeOffset = 3 * 60;  // s
constexpr auto fotoProgramTimeout = 6 * 60;          // s


auto InitializeEduProgramQueue() -> void
{
    eduProgramQueue.push_back({.programId = sensorProgramId,
                               .queueId = 0,
                               .startTime = initialProgramDelay + sensorProgramStartTimeOffset
                                          + utility::rodosUnixOffset / RODOS::SECONDS,
                               .timeout = sensorProgramTimeout});
    eduProgramQueue.push_back({.programId = fotoProgramId,
                               .queueId = 1,
                               .startTime = initialProgramDelay + fotoProgramStartTimeOffset
                                          + utility::rodosUnixOffset / RODOS::SECONDS,
                               .timeout = fotoProgramTimeout});
}


auto UpdateEduProgramQueueEntry(EduQueueEntry * entry) -> void
{
    entry->startTime += eduProgramQueuePeriod;

    if(entry->programId == fotoProgramId)
    {
        entry->queueId++;
    }
}


auto UpdateQueueIndex() -> void
{
    queueIndex++;

    if(queueIndex >= eduProgramQueue.size())
    {
        queueIndex = 0;
    }
}
}