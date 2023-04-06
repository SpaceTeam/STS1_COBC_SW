#include <Sts1CobcSw/CommandParser.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>

namespace sts1cobcsw
{
class HelloDummy : public RODOS::StaticThread<>
{
    void run() override
    {
        RODOS::PRINTF("Hello World!\n");
        RODOS::PRINTF("commandSize : %d\n", commandSize);
        RODOS::PRINTF("dataSize : %d\n", dataSize);
        RODOS::PRINTF("headerSize2 : %d\n", serial::serialSize<GsCommandHeader>);
        static_assert(commandSize == dataSize + serial::serialSize<GsCommandHeader>,
                      "Error in command size");
        etl::vector<serial::Byte, commandSize> command{};
        // Start character
        command.push_back(static_cast<Byte>('a'));
        // UTC
        command.push_back(static_cast<Byte>(0x80));
        command.push_back(static_cast<Byte>(0x00));
        command.push_back(static_cast<Byte>(0x92));
        command.push_back(static_cast<Byte>(0x65));
        // Command Id
        command.push_back(static_cast<Byte>(0x34));
        // Length
        command.push_back(static_cast<Byte>(0x14));
        command.push_back(static_cast<Byte>(0x00));

        // Queue entry 1 - ProgramId(i16)/QueueId(i16)/startTime(i32)/timeout(i16)
        command.push_back(static_cast<Byte>(0x01));
        command.push_back(static_cast<Byte>(0x00));
        command.push_back(static_cast<Byte>(0x10));
        command.push_back(static_cast<Byte>(0x00));
        command.push_back(static_cast<Byte>(0x01));
        command.push_back(static_cast<Byte>(0x00));
        command.push_back(static_cast<Byte>(0x10));
        command.push_back(static_cast<Byte>(0x00));
        command.push_back(static_cast<Byte>(0x01));
        command.push_back(static_cast<Byte>(0x00));

        // Queue entry 1 - ProgramId(i16)/QueueId(i16)/startTime(i32)/timeout(i16)
        command.push_back(static_cast<Byte>(0x02));
        command.push_back(static_cast<Byte>(0x00));
        command.push_back(static_cast<Byte>(0x20));
        command.push_back(static_cast<Byte>(0x00));
        command.push_back(static_cast<Byte>(0x02));
        command.push_back(static_cast<Byte>(0x00));
        command.push_back(static_cast<Byte>(0x20));
        command.push_back(static_cast<Byte>(0x00));
        command.push_back(static_cast<Byte>(0x02));
        command.push_back(static_cast<Byte>(0x00));


        while(not command.full())
        {
            command.push_back(Byte(0x00));
        }
        RODOS::PRINTF("Command Size = %d\n", command.size());
        DispatchCommand(command);

        RODOS::hwResetAndReboot();
    }
};
auto const helloDummy = HelloDummy();

}
