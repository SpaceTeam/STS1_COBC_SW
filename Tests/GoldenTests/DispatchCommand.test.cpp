#include <Sts1CobcSw/CommandParser.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <cstdint>


std::uint32_t printfMask = 0;


namespace sts1cobcsw
{
using serial::Byte;

periphery::Edu edu = periphery::Edu();


class HelloWorld : public RODOS::StaticThread<>
{
    void run() override
    {
        printfMask = 1;
        RODOS::PRINTF("Hello, World!\n");

        // Create command
        auto command = etl::vector<serial::Byte, commandSize>();

        // Start character
        command.push_back(static_cast<Byte>('a'));

        // UTC
        command.push_back(static_cast<Byte>(0x80));
        command.push_back(static_cast<Byte>(0x00));
        command.push_back(static_cast<Byte>(0x92));
        command.push_back(static_cast<Byte>(0x65));

        // Command ID
        command.push_back(static_cast<Byte>(0x34));

        // Length
        command.push_back(static_cast<Byte>(0x14));
        command.push_back(static_cast<Byte>(0x00));

        // Queue entry  1
        // Program ID : 1
        // Queue ID   : 16
        // Start time : 1048577
        // Timeout    : 2
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

        // Queue entry  2
        // Program ID : 2
        // Queue ID   : 32
        // Start time : 2097154
        // Timeout    : 2
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
            command.push_back(static_cast<Byte>(0x00));
        }
        // RODOS::PRINTF("Command Size = %d\n", command.size());
        DispatchCommand(command);


        RODOS::hwResetAndReboot();
    }
} helloWorld;
}
