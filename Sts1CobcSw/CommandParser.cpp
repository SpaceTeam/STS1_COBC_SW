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


namespace RODOS
{
#if defined(LINUX_SYSTEM)
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

constexpr auto rodosUnixOffset = 946'684'800 * RODOS::SECONDS;

// Helper function to print time to uart_stdout;
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
    // TODO : WIP

    auto targetIsCobc = true;
    ts::size_t position = 1_usize;


    // Read the 4 bytes value
    uint32_t utc = 0;
    util::CopyFrom(command, &position, &utc);
    // Convert it to 8 bytes
    auto utcStamp = static_cast<int64_t>(utc);
    // Convert it to nanoseconds.
    utcStamp = utcStamp * RODOS::SECONDS;

    // NOLINTNEXTLINE
    RODOS::PRINTF("Our timesamp is : %lld\n", utcStamp);

    // Set UTC :
    RODOS::sysTime.setUTC(utcStamp - rodosUnixOffset);

    constexpr auto typeIndex = 5;
    auto commandId = command[typeIndex];

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

    // TODO move this to a .hpp file
    static constexpr auto generalPurposeUpload = 0x01;
    static constexpr auto buildQueue = 0x02;
    static constexpr auto returnAvailableResults = 0x03;
    static constexpr auto returnSpecificResult = 0x04;
    static constexpr auto deleteFromCobc = 0x05;
    static constexpr auto listCobcFiles = 0x06;
    static constexpr auto listEduProgram = 0x07;

    void run() override
    {
        // Start Byte
        constexpr auto startByte = '$';
        // 1st January 2022, for testing purpose
        constexpr ts::int32_t utcStamp = 1640991600;
        // Type
        constexpr auto type = ts::narrow_cast<ts::int8_t>(generalPurposeUpload);
        // Length
        constexpr auto length = ts::narrow_cast<ts::int16_t>(1);
        // Value
        constexpr auto value = ts::narrow_cast<ts::int8_t>(42);
        // Stop Byte
        constexpr auto stopByte = '\n';
        uint8_t checksum = 0;

        // Trying to define length as sizeof(value.get()) results in an error
        // (this is not a constexpr) and a warning (sizeof usage considered suspicious).
        // So we have to check that :
        RODOS_ASSERT_IFNOT_RETURN_VOID(sizeof(value) == length.get())

        constexpr auto gsCommandSize = sizeof(startByte) + sizeof(utcStamp) + sizeof(type)
                                     + sizeof(length) + sizeof(value) + sizeof(checksum)
                                     + sizeof(stopByte);

        auto command = std::array<std::byte, gsCommandSize>{};
        auto position = 0_usize;
        util::CopyTo(command, &position, startByte);
        util::CopyTo(command, &position, utcStamp);
        util::CopyTo(command, &position, type);
        util::CopyTo(command, &position, length);
        util::CopyTo(command, &position, value);
        checksum = ComputeChecksum(std::span(command));
        util::CopyTo(command, &position, checksum);
        util::CopyTo(command, &position, stopByte);

        // Stop printing stuff
        RODOS::AT(RODOS::END_OF_TIME);

        TIME_LOOP(0, 2 * RODOS::SECONDS)
        {
            // RODOS::PRINTF("Writing to uart_stdout");
            util::WriteTo(&RODOS::uart_stdout, std::span(command));
        }
    }
} gsFaker;

}
