#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/Periphery/Enums.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw
{
using sts1cobcsw::serial::Byte;


auto edu = periphery::Edu();
auto uciUart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);


class ExecuteProgramTest : public RODOS::StaticThread<>
{
    void init() override
    {
        edu.Initialize();
    }


    void run() override
    {
        RODOS::PRINTF("\nEDU Command tester\n");
        RODOS::PRINTF("Commands:\n");
        RODOS::PRINTF("u: Update Time\n");
        RODOS::PRINTF("e: Execute Program\n");
        RODOS::PRINTF("g: Get Status\n");
        RODOS::PRINTF("r: Return Result\n");

        while(true)
        {
            RODOS::PRINTF("\nEnter a command\n");
            char command = 0_b;
            uciUart.read(&command, 1);

            // TODO: finish
            switch(command)
            {
                case 'u':
                    RODOS::PRINTF("\nUpdate Time\n");
                    RODOS::PRINTF("Enter a timestamp (2 bytes):\n");
                    serial::SerialBuffer<uint16_t> timestampBuffer = {};
                    uciUart.suspendUntilDataReady();
                    uciUart.ReadFrom(&uciUart, timestampBuffer);
                    auto timestamp = serial::Deserialize<uint16_t>(timestampBuffer);
                    auto errorCode = edu.UpdateTime(timestamp);
                    RODOS::PRINTF("Update Time to %d returned error code: %d\n",
                                  static_cast<int>(timestamp),
                                  static_cast<int>(errorCode));
                    break;

                case 'e':
                    RODOS::PRINTF("\nExecute Program\n");
                    RODOS::PRINTF("Enter the Program ID:\n");
                    RODOS::PRINTF("Enter the Queue ID:\n");
                    RODOS::PRINTF("Enter the timeout:\n");
                    break;

                case 'g':
                    break;

                case 'r':
                    break;

                default:
                    break;
            }
        }
    }
};


auto const executeProgramTest = ExecuteProgramTest();
}