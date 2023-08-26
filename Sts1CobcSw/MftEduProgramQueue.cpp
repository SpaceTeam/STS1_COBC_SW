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


constexpr auto initialProgramDelay = 25_i32;  // s, should be > eduBootTime
constexpr auto eduProgramQueuePeriod = 30;    // s

constexpr auto testProgramId = 1_u16;
constexpr auto initialTestProgramQueueId = 0U;
constexpr auto testProgramStartTimeOffset = 0;  // s
constexpr auto testProgramTimeout = 5_i16;      // s

constexpr auto sensorProgramId = 2_u16;
constexpr auto initialSensorProgramQueueId = 1U;
constexpr auto sensorProgramStartTimeOffset = 15_i32;  // s
constexpr auto sensorProgramTimeout = 6_i16;           // s

constexpr ts::int32_t rodosUnixOffsetSeconds =
    static_cast<int32_t>(utility::rodosUnixOffset / RODOS::SECONDS);


// Can be useful for tests
auto InitializeEduProgramQueue() -> void
{
    eduProgramQueue.clear();

    eduProgramQueue.push_back(
        {.programId = testProgramId,
         .queueId = initialTestProgramQueueId,
         .startTime = initialProgramDelay + testProgramStartTimeOffset + rodosUnixOffsetSeconds,
         .timeout = testProgramTimeout});

    eduProgramQueue.push_back(
        {.programId = sensorProgramId,
         .queueId = initialSensorProgramQueueId,
         .startTime = initialProgramDelay + sensorProgramStartTimeOffset + rodosUnixOffsetSeconds,
         .timeout = sensorProgramTimeout});
}


auto UpdateEduProgramQueueEntry(EduQueueEntry * entry) -> void
{
    entry->startTime += eduProgramQueuePeriod;
    if(entry->programId == sensorProgramId)
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
