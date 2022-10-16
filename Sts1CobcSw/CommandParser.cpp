#include <Sts1CobcSw/CobcCommands.hpp>
#include <Sts1CobcSw/CommandParser.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
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

#include <cstring>
#include <span>
#include <tuple>


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

// Number of nanoseconds between 1st January 1970 and 1st January 2000
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

//! @brief Given a time in seconds since January 1st 1970, return a time in nanoseconds since
//! January 1st 2000.
auto UnixToRodosTime(int32_t const unixTime)
{
    auto rodosTime = static_cast<int64_t>(unixTime);
    rodosTime = rodosTime * RODOS::SECONDS;
    rodosTime = rodosTime - rodosUnixOffset;
    return rodosTime;
}


enum class CommandType
{
    turnEduOn = '1',
    turnEduOff = '2',
    updateUtcOffset = '3',
    buildQueue = '4'
};

auto DispatchCommand(const etl::string<commandSize.get()> & command)
{
    auto targetIsCobc = true;
    ts::size_t position = 1_usize;
    int32_t utc = 0;
    CommandType commandId;

    util::CopyFrom(command, &position, &utc);
    RODOS::sysTime.setUTC(UnixToRodosTime(utc));
    PrintTime();

    util::CopyFrom(command, &position, &commandId);
    RODOS::PRINTF("command ID is character : %c\n", commandId);

    if(targetIsCobc)
    {
        switch(commandId)
        {
            case CommandType::turnEduOn:
            {
                TurnEduOn();
                return;
            }
            case CommandType::turnEduOff:
            {
                TurnEduOff();
                return;
            }
            case CommandType::updateUtcOffset:
            {
                UpdateUtcOffset();
                return;
            }
            case CommandType::buildQueue:
            {
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
                    uint16_t progId = 0;
                    util::CopyFrom(command, &position, &progId);
                    uint16_t queueId = 0;
                    util::CopyFrom(command, &position, &queueId);
                    uint32_t startTime = 0;
                    util::CopyFrom(command, &position, &startTime);
                    uint16_t timeout = 0;
                    util::CopyFrom(command, &position, &timeout);

                    auto queueEntry = QueueEntry{.programId = progId,
                                                 .queueId = queueId,
                                                 .startTime = startTime,
                                                 .timeout = timeout};
                    AddQueueEntry(queueEntry);
                }
                ResetQueueIndex();

                return;
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
    // TODO: fix this mess with type safe
    return static_cast<uint8_t>(std::accumulate(std::begin(data) + 1,
                                                std::end(data) - 2,
                                                0,
                                                [](auto sum, auto currentElement)
                                                {
                                                    auto x = static_cast<uint8_t>(currentElement);
                                                    return sum + x;
                                                }));
}
}
