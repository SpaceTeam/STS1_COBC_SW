#include <Sts1CobcSw/CommandParser.hpp>
#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <cstdint>


std::uint32_t printfMask = 0;


namespace sts1cobcsw
{
class DispatchCommandTest : public RODOS::StaticThread<>
{
public:
    DispatchCommandTest() : StaticThread("DispatchCommandTest")
    {
    }


private:
    void run() override
    {
        printfMask = 1;
        auto command = etl::vector<Byte, commandSize>();

        // Start character; doesn't matter which one since DispatchCommand() does not check it
        auto header = Serialize(GsCommandHeader{.startCharacter = 'a',
                                                .utc = 0x65920080,
                                                .commandId = 52,
                                                .length = 2 * serialSize<edu::QueueEntry>});
        command.insert(command.end(), header.begin(), header.end());

        auto entry1 = Serialize(
            edu::QueueEntry{.programId = 1, .timestamp = 16, .startTime = 1048577, .timeout = 1});
        command.insert(command.end(), entry1.begin(), entry1.end());

        auto entry2 = Serialize(
            edu::QueueEntry{.programId = 2, .timestamp = 32, .startTime = 2097154, .timeout = 2});
        command.insert(command.end(), entry2.begin(), entry2.end());

        while(not command.full())
        {
            command.push_back(0x00_b);
        }

        DispatchCommand(command);
        RODOS::hwResetAndReboot();
    }
} dispatchCommandTest;
}
