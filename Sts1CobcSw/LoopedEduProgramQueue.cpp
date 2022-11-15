#include <Sts1CobcSw/EduProgramQueue.hpp>
#include <Sts1CobcSw/LoopedEduProgramQueue.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
constexpr auto initialProgramDelay = 25;    // s; should be > eduBootTime
constexpr auto eduProgramQueuePeriod = 40;  // s

constexpr auto sensorProgramId = 0;
constexpr auto initialSensorProgramQueueId = 1;
constexpr auto sensorProgramStartTimeOffset = 0;  // s
constexpr auto sensorProgramTimeout = 3;          // s

constexpr auto fotoProgramId = 0;
constexpr auto initialFotoProgramQueueId = 5;
constexpr auto fotoProgramStartTimeOffset = 20;  // s
constexpr auto fotoProgramTimeout = 5;           // s


auto InitializeEduProgramQueue() -> void
{
    eduProgramQueue.push_back({.programId = sensorProgramId,
                               .queueId = initialSensorProgramQueueId,
                               .startTime = initialProgramDelay + sensorProgramStartTimeOffset
                                          + utility::rodosUnixOffset / RODOS::SECONDS,
                               .timeout = sensorProgramTimeout});
    eduProgramQueue.push_back({.programId = fotoProgramId,
                               .queueId = initialFotoProgramQueueId,
                               .startTime = initialProgramDelay + fotoProgramStartTimeOffset
                                          + utility::rodosUnixOffset / RODOS::SECONDS,
                               .timeout = fotoProgramTimeout});
}


auto UpdateEduProgramQueueEntry(EduQueueEntry * entry) -> void
{
    entry->startTime += eduProgramQueuePeriod;

    // if(entry->programId == fotoProgramId)
    // {
    //     entry->queueId++;
    // }
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