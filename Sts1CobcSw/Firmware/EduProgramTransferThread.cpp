#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/FileSystem/DirectoryIterator.hpp>
#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/Firmware/EduPowerManagementThread.hpp>
#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <utility>


namespace sts1cobcsw
{
namespace
{
constexpr auto stackSize = 1500U;
constexpr auto eduProgramTransferThreadInterval = 5 * s;


auto RemoveProgram(fs::Path const & file) -> void;
auto SendProgramsToEdu(fs::DirectoryIterator & fileIterator) -> void;
[[nodiscard]] auto IsEduCommunicationError(ErrorCode error) -> bool;


class EduProgramTransferThread : public RODOS::StaticThread<stackSize>
{
public:
    EduProgramTransferThread()
        : StaticThread("EduProgramQueueThread", eduProgramTransferThreadPriority)
    {}


private:
    void run() override
    {
        SuspendFor(totalStartupTestTimeout);  // Wait for the startup tests to complete
        SuspendFor(eduPowerManagementThreadStartDelay);
        DEBUG_PRINT("Starting EDU program transfer thread\n");
        TIME_LOOP(0, value_of(eduProgramTransferThreadInterval))
        {
            auto eduIsAlive = false;
            eduIsAliveBufferForProgramTransfer.get(eduIsAlive);
            if(edu::ProgramsAreAvailableOnCobc() and eduIsAlive)
            {
                DEBUG_PRINT("Programs available for transfer to EDU\n");
                auto makeIteratorResult = fs::MakeIterator(edu::programsDirectory);
                if(makeIteratorResult.has_error())
                {
                    DEBUG_PRINT("Failed to read EDU program directory: %s\n",
                                ToCZString(makeIteratorResult.error()));
                }
                else
                {
                    SendProgramsToEdu(makeIteratorResult.value());
                }
            }
            DEBUG_PRINT_STACK_USAGE();
        }
    }
} eduProgramQueueThread;


auto RemoveProgram(fs::Path const & file) -> void
{
    auto path = edu::programsDirectory;
    path.append("/");
    path.append(file);
    auto removeResult = fs::Remove(path);
    if(removeResult.has_error())
    {
        DEBUG_PRINT("Failed to remove transferred EDU program %s: %s\n",
                    path.c_str(),
                    ToCZString(removeResult.error()));
    }
}


auto SendProgramsToEdu(fs::DirectoryIterator & fileIterator) -> void
{
    for(auto fileInfoResult : fileIterator)
    {
        if(fileInfoResult.has_error())
        {
            DEBUG_PRINT("Failed to read a EDU program file: %s\n",
                        ToCZString(fileInfoResult.error()));
            continue;
        }
        auto const & fileInfo = fileInfoResult.value();
        if(fileInfo.type != fs::EntryType::file)
        {
            continue;
        }
        auto readProgramIdResult = edu::GetProgramId(fileInfo.name);
        if(readProgramIdResult.has_error())
        {
            continue;
        }
        DEBUG_PRINT("Sending program %s to EDU\n", fileInfo.name.c_str());
        auto program = edu::StoreProgramData{.programId = readProgramIdResult.value()};
        auto storeResult = edu::StoreProgram(program);
        if(storeResult.has_error())
        {
            auto error = storeResult.error();
            DEBUG_PRINT(
                "Failed to send program %s to EDU: %s\n", fileInfo.name.c_str(), ToCZString(error));
            if(not IsEduCommunicationError(error))  // error from filesystem (locked, to large, ...)
            {
                continue;
            }
            persistentVariables.Increment<"nEduCommunicationErrors">();
            ResetEdu();
            return;
        }
        RemoveProgram(fileInfo.name);
        DEBUG_PRINT("Removed EDU program %s\n", fileInfo.name.c_str());
    }
}


auto IsEduCommunicationError(ErrorCode error) -> bool
{
    if(error == ErrorCode::timeout)
    {
        return true;
    }
    return error >= ErrorCode::invalidAnswer and error < ErrorCode::full;
}
}
}
