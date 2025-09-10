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
#include <Sts1CobcSw/Vocabulary/ProgramId.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <charconv>
#include <cstdint>
#include <system_error>
#include <utility>


namespace sts1cobcsw
{
namespace
{
constexpr auto stackSize = 5000U;
constexpr auto eduProgramTransferThreadInterval = 5 * s;


[[nodiscard]] auto GetProgramId(fs::DirectoryInfo const & info) -> Result<ProgramId>;
auto HandleError(ErrorCode error) -> void;
auto RemoveProgram(fs::Path const & file) -> void;
auto SendProgramsToEdu(fs::DirectoryIterator & fileIterator) -> void;


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


auto GetProgramId(fs::DirectoryInfo const & info) -> Result<ProgramId>
{
    auto const fileAppendixLength = 4;  // .zip
    if(info.type != fs::EntryType::file)
    {
        return ErrorCode::invalidParameter;
    }
    if(info.name.size() > fileAppendixLength)
    {
        std::uint32_t value = 0;
        auto result = std::from_chars(
            info.name.data(), info.name.data() + (info.name.size() - fileAppendixLength), value);
        if(result.ec == std::errc{})
        {
            return sts1cobcsw::ProgramId(value);
        }
    }
    DEBUG_PRINT("Failed to get EDU program ID from file: %s\n", info.name.c_str());
    return ErrorCode::invalidParameter;
}


auto HandleError([[maybe_unused]] ErrorCode error) -> void
{
    DEBUG_PRINT("Failed to transfer program to EDU: %s\n", ToCZString(error));
    persistentVariables.Increment<"nEduCommunicationErrors">();
    ResetEdu();
}


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
        auto readProgramIdResult = GetProgramId(fileInfo);
        if(readProgramIdResult.has_error())
        {
            continue;
        }
        DEBUG_PRINT("Sending program %s to EDU\n", fileInfo.name.c_str());
        auto program = edu::StoreProgramData{.programId = readProgramIdResult.value()};
        auto storeResult = edu::StoreProgram(program);
        if(storeResult.has_error())
        {
            if(storeResult.error() == ErrorCode::fileLocked)
            {
                DEBUG_PRINT("Failed to send program %s to EDU, file is locked\n",
                            fileInfo.name.c_str());
                continue;
            }
            HandleError(storeResult.error());
            return;
        }
        RemoveProgram(fileInfo.name);
        DEBUG_PRINT("Removed EDU program %s\n", fileInfo.name.c_str());
    }
}
}
}
