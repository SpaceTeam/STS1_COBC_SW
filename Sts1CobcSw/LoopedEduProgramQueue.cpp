#include <Sts1CobcSw/EduProgramQueue.hpp>
#include <Sts1CobcSw/LoopedEduProgramQueue.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
// NOLINTBEGIN(misc-unused-using-decls)
using ts::operator""_u16;
using ts::operator""_i16;
using ts::operator""_i32;
// NOLINTEND(misc-unused-using-decls)

constexpr auto initialProgramDelay = 25_i32;  // s;should be > eduBootTime
constexpr auto eduProgramQueuePeriod = 45;    // s

constexpr auto sensorProgramId = 1_u16;
constexpr auto initialSensorProgramQueueId = 1U;
constexpr auto sensorProgramStartTimeOffset = 0_i32;  // s
constexpr auto sensorProgramTimeout = 6_i16;          // s

constexpr auto fotoProgramId = 2_u16;
constexpr auto initialFotoProgramQueueId = 1U;
constexpr auto fotoProgramStartTimeOffset = 15;  // s
constexpr auto fotoProgramTimeout = 5_i16;       // s

constexpr ts::int32_t rodosUnixOffsetSeconds =
    static_cast<int32_t>(utility::rodosUnixOffset / RODOS::SECONDS);

// Can be useful for tests
auto InitializeEduProgramQueue() -> void
{
    eduProgramQueue.push_back(
        {.programId = sensorProgramId,
         .queueId = initialSensorProgramQueueId,
         .startTime = initialProgramDelay + sensorProgramStartTimeOffset + rodosUnixOffsetSeconds,
         .timeout = sensorProgramTimeout});

    eduProgramQueue.push_back(
        {.programId = fotoProgramId,
         .queueId = initialFotoProgramQueueId,
         .startTime = initialProgramDelay + fotoProgramStartTimeOffset + rodosUnixOffsetSeconds,
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
