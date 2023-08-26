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


constexpr auto initialProgramDelay = 30_i32;  // s, should be > eduBootTime
constexpr auto eduProgramQueuePeriod = 14;    // s

constexpr auto program1Id = 0_u16;
constexpr auto initialProgram1QueueId = 123U;
constexpr auto program1StartTimeOffset = 0;  // s
constexpr auto program1Timeout = 2_i16;      // s

constexpr auto program2Id = 1_u16;
constexpr auto initialProgram2QueueId = 0U;
constexpr auto program2StartTimeOffset = 7_i32;  // s
constexpr auto program2Timeout = 2_i16;          // s

constexpr ts::int32_t rodosUnixOffsetSeconds =
    static_cast<int32_t>(utility::rodosUnixOffset / RODOS::SECONDS);


// Can be useful for tests
auto InitializeEduProgramQueue() -> void
{
    eduProgramQueue.clear();

    eduProgramQueue.push_back(
        {.programId = program1Id,
         .queueId = initialProgram1QueueId,
         .startTime = initialProgramDelay + program1StartTimeOffset + rodosUnixOffsetSeconds,
         .timeout = program1Timeout});

    eduProgramQueue.push_back(
        {.programId = program2Id,
         .queueId = initialProgram2QueueId,
         .startTime = initialProgramDelay + program2StartTimeOffset + rodosUnixOffsetSeconds,
         .timeout = program2Timeout});
}


auto UpdateEduProgramQueueEntry(EduQueueEntry * entry) -> void
{
    entry->startTime += eduProgramQueuePeriod;
    if(entry->programId == program2Id)
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
