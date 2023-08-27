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


constexpr auto initialProgramDelay = 35_i32;  // s, should be > eduBootTime
constexpr auto eduProgramQueuePeriod = 120;   // s

constexpr auto sensorProgramId = 0_u16;
constexpr auto initialSensorProgramQueueId = 0U;
constexpr auto sensorProgramStartTimeOffset = 0;  // s
constexpr auto sensorProgramTimeout = 5_i16;      // s

constexpr auto photoProgramId = 1_u16;
constexpr auto initialPhotoProgramQueueId = 200U;
constexpr auto photoProgramStartTimeOffset = 60_i32;  // s
constexpr auto photoProgramTimeout = 5_i16;           // s

constexpr ts::int32_t rodosUnixOffsetSeconds =
    static_cast<int32_t>(utility::rodosUnixOffset / RODOS::SECONDS);


// Can be useful for tests
auto InitializeEduProgramQueue() -> void
{
    eduProgramQueue.clear();

    eduProgramQueue.push_back(
        {.programId = sensorProgramId,
         .queueId = initialSensorProgramQueueId,
         .startTime = initialProgramDelay + sensorProgramStartTimeOffset + rodosUnixOffsetSeconds,
         .timeout = sensorProgramTimeout});

    eduProgramQueue.push_back(
        {.programId = photoProgramId,
         .queueId = initialPhotoProgramQueueId,
         .startTime = initialProgramDelay + photoProgramStartTimeOffset + rodosUnixOffsetSeconds,
         .timeout = photoProgramTimeout});
}


auto UpdateEduProgramQueueEntry(EduQueueEntry * entry) -> void
{
    entry->startTime += eduProgramQueuePeriod;
    entry->queueId++;
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
