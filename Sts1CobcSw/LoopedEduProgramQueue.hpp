#pragma once


namespace sts1cobcsw
{
struct EduQueueEntry;


auto InitializeEduProgramQueue() -> void;
auto UpdateEduProgramQueueEntry(EduQueueEntry * entry) -> void;
auto UpdateQueueIndex() -> void;
}