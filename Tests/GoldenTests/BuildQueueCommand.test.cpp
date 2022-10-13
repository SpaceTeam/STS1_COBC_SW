#include <Sts1CobcSw/EduProgramQueueThread.hpp>

#include <Sts1CobcSw/Util/Util.hpp>
#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/string.h>


uint32_t printfMask = 0;

namespace sts1cobcsw
{
namespace ts = type_safe;
using ts::operator""_usize;


//! @brief Print utc system time in human readable format
void PrintTime()
{
    int32_t year = 0;
    int32_t month = 0;
    int32_t day = 0;
    int32_t hour = 0;
    int32_t min = 0;
    double sec = 0;

    auto sysUTC = RODOS::sysTime.getUTC();
    RODOS::TimeModel::localTime2Calendar(sysUTC, year, month, day, hour, min, sec);
    RODOS::PRINTF("DateUTC(DD/MM/YYYY HH:MIN:SS) : %ld/%ld/%ld %ld:%ld:%f\n",
                  day,    // NOLINT
                  month,  // NOLINT
                  year,   // NOLINT
                  hour,   // NOLINT
                  min,    // NOLINT
                  sec);
}

class BuildQueueCommandTest : public RODOS::StaticThread<>
{
    void run() override
    {
        printfMask = 1;
        constexpr auto beaconSize = 20;
        constexpr char startByte = '$';
        // 1st January 2023, utc Epoch.
        constexpr int32_t date = 1'672'531'200;
        constexpr int8_t type = 0x34;
        int16_t length = 30; //NOLINT
        int16_t progId = 12; //NOLINT
        int16_t queueId = 23; //NOLINT
        int32_t startTime = 1'672'531'260; //NOLINT
        int16_t maxTime = 10; //NOLINT


        constexpr auto rodosUnixOffset = 946'684'800 * RODOS::SECONDS;
        auto beacon = std::array<std::byte, beaconSize>{};

        ts::size_t pos = 0_usize;
        util::CopyTo(beacon, &pos, startByte);
        util::CopyTo(beacon, &pos, date);
        util::CopyTo(beacon, &pos, type);
        util::CopyTo(beacon, &pos, length);
        for(int i = 0; i < 3; ++i)
        {
            progId += i;
            queueId += i;
            startTime += i;
            maxTime += i;
            util::CopyTo(beacon, &pos, progId);
            util::CopyTo(beacon, &pos, queueId);
            util::CopyTo(beacon, &pos, startTime);
            util::CopyTo(beacon, &pos, maxTime);
        }

        ts::size_t position = 1_usize;
        uint32_t utc = 0;
        util::CopyFrom(std::span(beacon), &position, &utc);
        auto utcStamp = static_cast<int64_t>(utc) * RODOS::SECONDS;
        RODOS::PRINTF("Our timesamp is : %lld\n", utcStamp);

        // Set UTC :
        RODOS::sysTime.setUTC(utcStamp - rodosUnixOffset);
        PrintTime();

        // 1 byte for type index
        int8_t commandId = 0;
        util::CopyFrom(std::span(beacon), &position, &commandId);
        RODOS::PRINTF("command ID is character : %c\n", commandId);

        switch(commandId)
        {
            case '1':
            {
                return;
            }
            case '2':
            {
                return;
            }
            case '3':
            {
                return;
            }
            case '4':
            {
                // Program ID 	: 2 bytes, according to EDU PDD 6.1.1
                // Queue ID 	: 2 bytes, according to EDU PDD 6.1.2
                // Start Time 	: 4 bytes, EPOCH time
                // Timeout 		: 2 bytes, according to EDU PDD 6.1.2
                constexpr auto queueEntrySize = 10;
                int16_t qlength = 0;
                util::CopyFrom(std::span(beacon), &position, &qlength);
                RODOS::PRINTF("Length of data is %d\n", qlength);

                if(qlength % queueEntrySize != 0)
                {
                    break;
                }
                auto const nbQueueEntries = length / queueEntrySize;
                RODOS::PRINTF("Number of queue entries : %d\n", nbQueueEntries);


                // AddQueueEntry(std::tie(progId, queueId, startTime, maxRunTime));

                // BuildQueue();

                RODOS::hwResetAndReboot();
                return;
            }

            default:
            {
                break;
            }
        }


        RODOS::PRINTF("*Error, invalid command*\n");
        RODOS::hwResetAndReboot();
    }
};

auto const buildQueueCommandTest = BuildQueueCommandTest();
}
