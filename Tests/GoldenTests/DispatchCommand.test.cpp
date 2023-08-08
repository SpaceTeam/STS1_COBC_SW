#include <Sts1CobcSw/CommandParser.hpp>
#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <cstdint>


std::uint32_t printfMask = 0;


namespace sts1cobcsw
{
using serial::Byte;
using serial::operator""_b;
using type_safe::operator""_u16;
using type_safe::operator""_i16;


edu::Edu eduUnit = edu::Edu();


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
        auto header =
            serial::Serialize(GsCommandHeader{.startCharacter = 'a',
                                              .utc = 0x65920080,
                                              .commandId = 52,
                                              .length = 2 * serial::serialSize<edu::QueueEntry>});
        command.insert(command.end(), std::begin(header), std::end(header));

        auto entry1 = serial::Serialize(edu::QueueEntry{
            .programId = 1_u16, .queueId = 16_u16, .startTime = 1048577, .timeout = 1_i16});
        command.insert(command.end(), entry1.begin(), entry1.end());

        auto entry2 = serial::Serialize(edu::QueueEntry{
            .programId = 2_u16, .queueId = 32_u16, .startTime = 2097154, .timeout = 2_i16});
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
