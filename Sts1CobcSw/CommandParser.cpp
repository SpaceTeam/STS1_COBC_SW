#include <Sts1CobcSw/CobcCommands.hpp>
#include <Sts1CobcSw/CommandParser.hpp>
#include <Sts1CobcSw/Topics.hpp>

#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Util/Util.hpp>
#include <type_safe/index.hpp>
#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/map.h>
#include <etl/string.h>
#include <etl/string_view.h>

#include <cstring>


namespace RODOS
{
// Include the following line to be able to read from UART when compiling for STM32
// NOLINTNEXTLINE(readability-identifier-naming)
// extern HAL_UART uart_stdout;

// On linux :
// NOLINTNEXTLINE(readability-identifier-naming)
HAL_UART uart_stdout(RODOS::UART_IDX2);
}


namespace ts = type_safe;

using ts::operator""_u8;
using ts::operator""_i32;
using ts::operator""_usize;


namespace sts1cobcsw
{
auto DispatchCommand(const etl::string<commandSize.get()> & command)
{
    auto targetIsCobc = command[1] == '0';
    auto commandId = command[2];

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


// startByte + 1 Id byte + 4 data bytes + endByte
constexpr auto dataFrameSize = 1_usize + 1_usize + sizeof(int32_t) + 1_usize;

auto ParseEduDataFrame(const etl::string<dataFrameSize.get()> & dataFrame)
{
    RODOS::PRINTF("Entering ParseEduDataFrame() ...\n");
    // Do nothing
    auto index = 1_usize;
    auto id = 0_u8;
    auto data = 0_i32;

    util::CopyFrom(dataFrame, &index, &id);
    util::CopyFrom(dataFrame, &index, &data);

    constexpr auto temperatureId = 1;
    constexpr auto accelerationXId = 2;
    constexpr auto accelerationYId = 3;
    constexpr auto accelerationZId = 4;
    constexpr auto brightnessId = 5;

    //    PRINTF("Received data (%ld) at id = %d\n",data.get(), id.get());

    switch(id.get())
    {
        case temperatureId:
            temperatureTopic.publish(data);
            break;
        case accelerationXId:
            accelerationXTopic.publish(data);
            break;
        case accelerationYId:
            accelerationYTopic.publish(data);
            break;
        case accelerationZId:
            accelerationZTopic.publish(data);
            break;
        case brightnessId:
            brightnessTopic.publish(data);
            break;
        default:;
            // Too bad
    }
}


class EduReaderThread : public RODOS::StaticThread<>
{
  public:
    EduReaderThread() : StaticThread("EduReaderThread")
    {
    }
  private:
    void init() override
    {
        constexpr auto baudrate = 9'600;
        eduUart.init(baudrate);
    }

    void run() override
    {
        RODOS::PRINTF("Entering EDU Data reader\n");
        constexpr auto startCharacter = '?';

        auto eduDataFrame = etl::string<dataFrameSize.get()>();
        ts::bool_t startWasDetected = false;
        while(true)
        {
            char readCharacter = 0;
            auto nReadCharacters = ts::size_t(eduUart.read(&readCharacter, 1));
            if(nReadCharacters != 0U)
            {
                RODOS::PRINTF("%c\n", readCharacter);
                if(readCharacter == startCharacter)
                {
                    startWasDetected = true;
                    eduDataFrame.clear();
                    eduDataFrame += startCharacter;
                }
                else if(startWasDetected)
                {
                    eduDataFrame += readCharacter;
                    if(eduDataFrame.full())
                    {
                        // Command full
                        // TODO maybe check that endbyte is correct
                        ParseEduDataFrame(eduDataFrame);
                        startWasDetected = false;
                    }
                }
            }
            eduUart.suspendUntilDataReady();
        }
    }
} eduReaderThread;


}
