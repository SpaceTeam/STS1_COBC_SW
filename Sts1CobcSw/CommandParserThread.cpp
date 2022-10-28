#include <Sts1CobcSw/EduProgramQueue.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <type_safe/index.hpp>
#include <type_safe/narrow_cast.hpp>
#include <type_safe/types.hpp>

#include <rodos-assert.h>
#include <timemodel.h>

#include <rodos_no_using_namespace.h>

#include <etl/string.h>

#include <cstdint>
#include <cstring>
#include <span>


namespace RODOS
{
// NOLINTNEXTLINE
extern HAL_UART uart_stdout;
}

namespace sts1cobcsw
{
namespace ts = type_safe;
using ts::operator""_i16;
using ts::operator""_usize;


enum CommandId : char
{
    turnEduOn = '1',
    turnEduOff = '2',
    buildQueue = '4',
};


// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 4'000U;
constexpr std::size_t commandSize = 30;
constexpr std::size_t queueEntrySize =
    sizeof(EduQueueEntry::programId) + sizeof(EduQueueEntry::queueId)
    + sizeof(EduQueueEntry::startTime) + sizeof(EduQueueEntry::timeout);


// TODO: Use serial library instead
template<std::size_t size>
auto CopyFrom(etl::string<size> const & buffer, ts::size_t * position, auto * value) -> void;
auto ParseAndAddQueueEntries(etl::string<commandSize> const & command) -> void;
auto DispatchCommand(etl::string<commandSize> const & command) -> void;


class CommandParserThread : public RODOS::StaticThread<stackSize>
{
public:
    CommandParserThread() : StaticThread("CommandParserThread")
    {
    }

private:
    void init() override
    {
    }


    void run() override
    {
        constexpr auto startCharacter = '$';

        auto command = etl::string<commandSize>();
        ts::bool_t startWasDetected = false;
        while(true)
        {
            char readCharacter = 0;
            // This needs to be abstracted away because "IRL" we receive commands via the RF module
            ts::size_t nReadCharacters =
                RODOS::uart_stdout.read(&readCharacter, sizeof(readCharacter));
            if(nReadCharacters != 0U)
            {
                // RODOS::PRINTF("Read a character : %c\n", readCharacter);
                if(readCharacter == startCharacter)
                {
                    startWasDetected = true;
                    command.clear();
                    command += startCharacter;
                }
                else if(startWasDetected)
                {
                    command += readCharacter;
                    // Every command has the same size
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


auto DispatchCommand(etl::string<commandSize> const & command) -> void
{
    ts::size_t position = 1_usize;

    // TODO: Why is this here? It has nothing to do with command dispatching
    std::int32_t utc = 0;
    CopyFrom(command, &position, &utc);
    RODOS::sysTime.setUTC(utility::UnixToRodosTime(utc));
    utility::PrintTime();

    char commandId = 0;
    CopyFrom(command, &position, &commandId);
    RODOS::PRINTF("Command ID character : %c\n", commandId);

    auto length = 0_i16;
    CopyFrom(command, &position, &length);
    RODOS::PRINTF("Length of data : %d\n", length.get());

    auto targetIsCobc = true;
    if(targetIsCobc)
    {
        switch(commandId)
        {
            case CommandId::turnEduOn:
            {
                edu.TurnOn();
                return;
            }
            case CommandId::turnEduOff:
            {
                edu.TurnOff();
                return;
            }
            case CommandId::buildQueue:
            {
                // TODO: This should be a function Build/BuildNew/Update/OverwriteEduQueue() or
                // something like that
                RODOS::PRINTF("Entering build queue command parsing\n");

                auto const nbQueueEntries = length.get() / static_cast<int>(queueEntrySize);
                RODOS::PRINTF("Number of queue entries : %d\n", nbQueueEntries);

                // Erase all previous entries in the EDU program queue
                eduProgramQueue.clear();

                ParseAndAddQueueEntries(
                    command.substr(position.get(), static_cast<std::size_t>(length.get())));

                // Reset queue index
                queueIndex = 0U;
                RODOS::PRINTF("Queue index reset. Current size of EDU program queue is %d.\n",
                              static_cast<int>(eduProgramQueue.size()));

                ResumeEduQueueThread();
                return;
            }
            default:
            {
                RODOS::PRINTF("*Error, invalid command*\n");
                return;
            }
        }
    }
}


// TODO: Use Deserialize instead of CopyFrom
auto ParseAndAddQueueEntries(etl::string<commandSize> const & command) -> void
{
    auto const nQueueEntries = command.size() / queueEntrySize;
    eduProgramQueue.resize(nQueueEntries);

    ts::size_t position = 0U;

    for(auto & entry : eduProgramQueue)
    {
        std::uint16_t programId = 0;
        CopyFrom(command, &position, &programId);
        RODOS::PRINTF("Prog ID      : %d\n", static_cast<int>(programId));
        std::uint16_t queueId = 0;
        CopyFrom(command, &position, &queueId);
        RODOS::PRINTF("Queue ID     : %d\n", static_cast<int>(queueId));
        std::int32_t startTime = 0;
        CopyFrom(command, &position, &startTime);
        RODOS::PRINTF("Start Time   : %d\n", static_cast<int>(startTime));
        std::int16_t timeout = 0;
        CopyFrom(command, &position, &timeout);
        RODOS::PRINTF("Timeout      : %d\n", static_cast<int>(timeout));

        entry = EduQueueEntry{
            .programId = programId, .queueId = queueId, .startTime = startTime, .timeout = timeout};
    }
}


//! @brief Copy a value from a buffer to a variable.
//!
//! During the process, the position parameter is updated, so that one can chain multiple calls to
//! CopyFrom(). The size of the variable that must be copied from the buffer is the size of the
//! value parameter.
//!
//! @param buffer The buffer our data is copied from.
//! @param position The position in the buffer our data is copied from.
//! @param value The variable that will hold our copied value.
// TODO: Remove this function. Use the serial library instead.
template<std::size_t size>
auto CopyFrom(etl::string<size> const & buffer, ts::size_t * const position, auto * value) -> void
{
    auto newPosition = *position + sizeof(*value);
    std::memcpy(value, &buffer[(*position).get()], sizeof(*value));
    *position = newPosition;
}
}
