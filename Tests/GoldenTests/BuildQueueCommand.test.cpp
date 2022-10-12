#include <Sts1CobcSw/EduProgramQueueThread.hpp>

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/string.h>


uint32_t printfMask = 0;

namespace sts1cobcsw
{
namespace ts = type_safe;
using ts::operator""_usize;


class HelloDummy : public RODOS::StaticThread<>
{
    void run() override
    {
        printfMask = 1;

        auto targetIsCobc = true;
        ts::size_t position = 1_usize;


        // Read the 4 bytes value
        uint32_t utc = 0;
        // util::CopyFrom(command, &position, &utc);
        // Convert it to 8 bytes
        // auto utcStamp = static_cast<int64_t>(utc);
        int64_t utcStamp = 1664981524;
        // Convert it to nanoseconds.
        utcStamp = utcStamp * RODOS::SECONDS;

        // NOLINTNEXTLINE
        // RODOS::PRINTF("Our timesamp is : %lld\n", utcStamp);

        // Set UTC :
        // RODOS::sysTime.setUTC(utcStamp - rodosUnixOffset);

        // constexpr auto typeIndex = 5;
        // auto commandId = command[typeIndex];

        // RODOS::PRINTF("command ID is character : %c\n", commandId);

        /*
        if(targetIsCobc)
        {
            switch(commandId)
            {
                case '1':
                {
                    TurnEduOn();
                    return;
                }
                case '2':
                {
                    TurnEduOff();
                    return;
                }
                case '3':
                {
                    UpdateUtcOffset();
                    return;
                }
                case '4':
                {
                    // Program ID 	: 2 bytes, according to EDU PDD 6.1.1
                    // Queue ID 	: 2 bytes, according to EDU PDD 6.1.2
                    // Start Time 	: 4 bytes, EPOCH time
                    // Timeout 		: 2 bytes, according to EDU PDD 6.1.2
                    BuildQueue();

                    return;
                }

                default:
                {
                    break;
                }
            }
        }

        RODOS::PRINTF("*Error, invalid command*\n");
        */
        hwResetAndReboot();
    }
};

auto const helloDummy = HelloDummy();
}
