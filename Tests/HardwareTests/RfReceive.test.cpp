#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <span>


namespace sts1cobcsw
{
using RODOS::PRINTF;
using sts1cobcsw::serial::Byte;
using sts1cobcsw::serial::operator""_b;


class RfReceiveTest : public RODOS::StaticThread<>
{
public:
    RfReceiveTest() : StaticThread("RfReceiveTest")
    {
    }

private:
    void init() override
    {
        RODOS::PRINTF("Hello RfReceive\n");
    }


    void run() override
    {
        periphery::rf::Initialize(periphery::rf::TxType::packet);
        RODOS::PRINTF("Si4463 initialized\n");

        // auto testData = std::to_array<Byte>({0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b});
        constexpr auto nReceiveRounds = 100;
        auto waitTime = 1 * RODOS::SECONDS;
        for(auto i = 0; i < nReceiveRounds; ++i)
        {
            RODOS::PRINTF("Receiving...\n");
            auto receiveBuffer = periphery::rf::ReceiveTestData();
            RODOS::PRINTF("Received:\n");
            for(auto && element : receiveBuffer)
            {
                RODOS::PRINTF("%x", element);
            }
        }

        RODOS::PRINTF("Done\n");
    }
} rfReceiveTest;
}