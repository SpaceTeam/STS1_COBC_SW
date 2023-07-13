#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/Periphery/RfNames.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <span>
#include <string_view>


namespace sts1cobcsw
{
using RODOS::PRINTF;
using sts1cobcsw::serial::Byte;

auto uciUart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);
std::string_view mountainFieldTestMessage = "OE1XST PORTABLE STS1 TEST";
auto const * shortMessage = "Hello from STS1!";
auto const * longMessage =
    "STS1 says:\n"
    "Westbahnhof Mariahilfer Strasse, Umsteigen zu den Linien U3, 52 und 58, sowie zum "
    "Flughafenbus.\n"
    "Ja ja ja ja, U3, am Start, U3 Supermarkt, U3, am Start, U3 Supermarkt, U3 am Start, U3 "
    "Supermarkt, U3 am Start, U3 Supermarkt.\n"
    "Ich bin beim U3 Supermarkt, weil heut ist schon wieder Sonntag. Ich kauf ein Club Mate ein, "
    "oder vielleicht 2, oder vielleicht auch 3, ja.\n";

class RfTest : public RODOS::StaticThread<>
{
public:
    RfTest() : StaticThread("RfTest")
    {
    }

private:
    void init() override
    {
        PRINTF("Hello RfTest\n");
        constexpr auto uartBaudRate = 115200;
        uciUart.init(uartBaudRate);
    }


    void run() override
    {
        periphery::rf::Initialize(periphery::rf::TxType::morse);
        PRINTF("Si4463 initialized\n");
        PRINTF("Checking part info\n");
        auto correctPartInfo = 0x4463;
        auto receivedPartInfo = periphery::rf::GetPartInfo();
        if(receivedPartInfo == correctPartInfo)
        {
            PRINTF("Correct part info was returned\n");
        }
        else
        {
            PRINTF("Incorrect part info\n");
        }
        PRINTF("Entering transmission test loop\n");

        while(true)
        {
            PRINTF("What do you want to test?\n");
            PRINTF("\tm: morsing\n");
            PRINTF("\tt: transmiting a packet\n");
            PRINTF("\tr: receiving\n");
            auto readBuffer = std::array<char, 1>{0};
            hal::ReadFrom(&uciUart, std::span<char, 1>(readBuffer));
            auto nMorses = 5;
            auto nTransmissions = 100;
            auto pauseDuration = 1 * RODOS::SECONDS;
            switch(readBuffer[0])
            {
                case 'm':
                    periphery::rf::SetTxType(periphery::rf::TxType::morse);

                    PRINTF("Morsing %d times\n", nMorses);
                    for(auto i = 0; i < nMorses; ++i)
                    {
                        PRINTF("Morsing...\n");
                        periphery::rf::Morse(mountainFieldTestMessage);
                        RODOS::AT(RODOS::NOW() + pauseDuration);
                    }
                    PRINTF("Morsing done\n");
                    break;

                case 't':
                    periphery::rf::SetTxType(periphery::rf::TxType::packet);

                    for(auto i = 0; i < nTransmissions; ++i)
                    {
                        RODOS::PRINTF("Transmitting...\n");
                        periphery::rf::TransmitData((uint8_t *)(mountainFieldTestMessage.data()), mountainFieldTestMessage.length());
                        RODOS::AT(RODOS::NOW() + pauseDuration);
                    }

                    RODOS::PRINTF("Packet done\n");
                    break;

                case 'r':
                    PRINTF("Not implemented\n");
                    break;

                default:
                    PRINTF("Unused character\n");
                    break;
            }
        }
    }
} rfTest;
}