#include <Sts1CobcSw/CobcCommands.hpp>
#include <Sts1CobcSw/CommandParser.hpp>
#include <Sts1CobcSw/Topics.hpp>

#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Util/Util.hpp>
#include <type_safe/index.hpp>
#include <type_safe/narrow_cast.hpp>
#include <type_safe/types.hpp>

#include <rodos-assert.h>
#include <timemodel.h>

#include <rodos_no_using_namespace.h>

#include <etl/map.h>
#include <etl/string.h>
#include <etl/string_view.h>

#include <cinttypes>
#include <cstring>
#include <span>


namespace RODOS
{
#if defined(LINUX_SYSTEM)
// TODO: remove this
// NOLINTNEXTLINE(readability-identifier-naming)
HAL_UART uart_stdout(RODOS::UART_IDX2);
#elif defined(GENERIC_SYSTEM)
// NOLINTNEXTLINE(readability-identifier-naming)
extern HAL_UART uart_stdout;
#endif
}

namespace sts1cobcsw
{
namespace ts = type_safe;
using ts::operator""_usize;


// Number of seconds between 1st January 1970 and 1st January 2000
constexpr auto rodosUnixOffset = 946'684'800 * RODOS::SECONDS;

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


auto DispatchCommand(const etl::string<commandSize.get()> & command)
{
    auto targetIsCobc = true;
    ts::size_t position = 1_usize;
    uint32_t utc = 0;
    int8_t commandId = 0;

    // Read the 4 bytes value
    util::CopyFrom(command, &position, &utc);
    // Convert it to 8 bytes
    auto utcStamp = static_cast<int64_t>(utc);
    // Convert it to nanoseconds.
    utcStamp = utcStamp * RODOS::SECONDS;

    // NOLINTNEXTLINE
    RODOS::PRINTF("Our timesamp is : %lld\n", utcStamp);

    // Set UTC :
    RODOS::sysTime.setUTC(utcStamp - rodosUnixOffset);

    // Check that system UTC is correct
    PrintTime();

    util::CopyFrom(command, &position, &commandId);
    RODOS::PRINTF("command ID is character : %c\n", commandId);

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
                constexpr auto queueEntrySize = 10;

                int16_t length = 0;
                util::CopyFrom(command, &position, &length);

                if(length % queueEntrySize != 0)
                {
                    break;
                }

                auto const nbQueueEntries = length / queueEntrySize;
                RODOS::PRINTF("Number of queue entries : %d", nbQueueEntries);

                for(auto i = 0; i < nbQueueEntries; ++i)
                {
                    int16_t progId = 0;
                    util::CopyFrom(command, &position, &progId);
                    int16_t queueId = 0;
                    util::CopyFrom(command, &position, &queueId);
                    int64_t startTime = 0;
                    util::CopyFrom(command, &position, &startTime);
                    int16_t maxRunTime = 0;
                    util::CopyFrom(command, &position, &maxRunTime);

                    // TODO: AddQueueEntry(std::tie(progId, queueId, startTime, maxRunTime));
                }

                return;
            }

            default:
            {
                break;
            }
        }
    }

    RODOS::PRINTF("*Error, invalid command*\n");
}


class CommandParserThread : public RODOS::StaticThread<>
{
    void init() override
    {
        eduEnabledGpio.init(/*isOutput=*/true, 1, 0);
    }

    void run() override
    {
        constexpr auto startCharacter = '$';

        auto command = etl::string<commandSize.get()>();
        ts::bool_t startWasDetected = false;
        while(true)
        {
            char readCharacter = 0;
            auto nReadCharacters = ts::size_t(RODOS::uart_stdout.read(&readCharacter, 1));
            if(nReadCharacters != 0U)
            {
                RODOS::PRINTF("Read a character : %c\n", readCharacter);
                if(readCharacter == startCharacter)
                {
                    startWasDetected = true;
                    command.clear();
                    command += startCharacter;
                }
                else if(startWasDetected)
                {
                    command += readCharacter;
                    if(command.full())
                    {
                        DispatchCommand(command);
                        startWasDetected = false;
                    }
                }
            }
            RODOS::uart_stdout.suspendUntilDataReady();
        }
    }
} commandParserThread;

template<std::size_t size>
auto ComputeChecksum(std::span<std::byte, size> data)
{
    static_assert(size >= 3,
                  "The size of 'beacon' must be >= 3 because the start, stop and "
                  "checksum bytes are not included in the computation.");
    // TODO fix this mess with type safe
    return static_cast<uint8_t>(std::accumulate(std::begin(data) + 1,
                                                std::end(data) - 2,
                                                0,
                                                [](auto sum, auto currentElement)
                                                {
                                                    auto x = static_cast<uint8_t>(currentElement);
                                                    return sum + x;
                                                }));
}

class GsFaker : public RODOS::StaticThread<>
{
    // Proposal for the command format :
    //  Start Byte                                  -> 1 byte
    //  Timestamp                                   -> 4 bytes
    //  Type (or Tag, what kind of command this is) -> 1 byte
    //  Length (the length of the data)             -> 2 bytes
    //  Value (the actual value)                    -> Length bytes
    //  checksum                                    -> 1 byte
    //  Stop Byte                                   -> 1 byte

    // For instance, a few tags
    // 01 : General Purpose Upload.
    // 02 : Build Queue.
    // 03 : Return List of available results.
    // 04 : Return specific results.
    // 05 : Delete file from COBC file system.
    // 06 : Return list of files in the COBC file system.
    // 07 : Return list of programs on EDU.

    void run() override
    {
        // Create a false command and parse it directly
        // DispatchCommandTest();
        RODOS::hwResetAndReboot();

        }
} gsFaker;
}
