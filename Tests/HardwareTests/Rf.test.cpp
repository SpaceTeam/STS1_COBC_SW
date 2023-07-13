#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/Periphery/RfNames.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <Tests/HardwareTests/Utility.hpp>

#include <rodos_no_using_namespace.h>

#include <span>
#include <string_view>


namespace sts1cobcsw
{
using RODOS::PRINTF;
using sts1cobcsw::serial::Byte;
using std::literals::operator""sv;

auto uciUart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);

auto callSign = "OE1XST"sv;
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
        PRINTF("\nRF test\n\n");

        periphery::rf::Initialize(periphery::rf::TxType::morse);
        PRINTF("RF module initialized\n");

        PRINTF("\n");
        auto correctPartInfo = 0x4463;
        auto partInfo = periphery::rf::GetPartInfo();
        PRINTF("Part info: 0x%4x == 0x%4x\n", partInfo, correctPartInfo);
        Check(partInfo == correctPartInfo);

        while(true)
        {
            PRINTF("\n");
            PRINTF("What do you want to test?\n");
            PRINTF("  m: morsing\n");
            PRINTF("  t: transmiting a packet\n");
            PRINTF("  r: receiving\n");

            auto command = std::array<char, 1>{0};
            hal::ReadFrom(&uciUart, std::span(command));
            PRINTF("\n");

            auto pauseDuration = 1 * RODOS::SECONDS;
            switch(command[0])
            {
                case 'm':
                {
                    auto nMorses = 5;
                    periphery::rf::SetTxType(periphery::rf::TxType::morse);
                    PRINTF("Morsing call sign 'OE1XST' %d times\n", nMorses);
                    for(auto i = 0; i < nMorses; ++i)
                    {
                        PRINTF("Morsing ...\n");
                        periphery::rf::Morse(callSign);
                        RODOS::AT(RODOS::NOW() + pauseDuration);
                    }
                    break;
                }
                case 't':
                {
                    auto nTransmissions = 5;
                    periphery::rf::SetTxType(periphery::rf::TxType::packet);
                    PRINTF("Transmitting call sign 'OE1XST' %d times\n", nTransmissions);
                    for(auto i = 0; i < nTransmissions; ++i)
                    {
                        RODOS::PRINTF("Transmitting...\n");
                        periphery::rf::TransmitData(
                            reinterpret_cast<std::uint8_t const *>(callSign.data()),
                            callSign.length());
                        RODOS::AT(RODOS::NOW() + pauseDuration);
                    }
                    break;
                }
                case 'r':
                {
                    PRINTF("Not implemented\n");
                    break;
                }
                default:
                {
                    PRINTF("Unused character\n");
                    break;
                }
            }
        }
    }
} rfTest;
}