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
using serial::operator""_b;


periphery::Edu edu = periphery::Edu();


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

        // Create command
        auto command = etl::vector<serial::Byte, commandSize>();

        // Start character
        command.push_back(static_cast<Byte>('a'));

        // UTC
        command.push_back(0x80_b);
        command.push_back(0x00_b);
        command.push_back(0x92_b);
        command.push_back(0x65_b);

        // Command ID
        command.push_back(0x34_b);

        // Length
        command.push_back(0x14_b);
        command.push_back(0x00_b);

        // Queue entry  1
        // Program ID : 1
        // Queue ID   : 16
        // Start time : 1048577
        // Timeout    : 2
        command.push_back(0x01_b);
        command.push_back(0x00_b);
        command.push_back(0x10_b);
        command.push_back(0x00_b);
        command.push_back(0x01_b);
        command.push_back(0x00_b);
        command.push_back(0x10_b);
        command.push_back(0x00_b);
        command.push_back(0x01_b);
        command.push_back(0x00_b);

        // Queue entry  2
        // Program ID : 2
        // Queue ID   : 32
        // Start time : 2097154
        // Timeout    : 2
        command.push_back(0x02_b);
        command.push_back(0x00_b);
        command.push_back(0x20_b);
        command.push_back(0x00_b);
        command.push_back(0x02_b);
        command.push_back(0x00_b);
        command.push_back(0x20_b);
        command.push_back(0x00_b);
        command.push_back(0x02_b);
        command.push_back(0x00_b);

        while(not command.full())
        {
            command.push_back(0x00_b);
        }
        // RODOS::PRINTF("Command Size = %d\n", command.size());
        DispatchCommand(command);

        RODOS::hwResetAndReboot();
    }
} dispatchCommandTest;
}
