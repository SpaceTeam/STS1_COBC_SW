#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <span>


namespace sts1cobcsw
{
using RODOS::PRINTF;
using sts1cobcsw::serial::Byte;
using sts1cobcsw::serial::operator""_b;


class RfTransmitTest : public RODOS::StaticThread<>
{
public:
    RfTransmitTest() : StaticThread("RfTransmitTest")
    {
    }

private:
    void init() override
    {
        RODOS::PRINTF("Hello RfTransmit\n");
    }


    void run() override
    {
        periphery::rf::Initialize();
        RODOS::PRINTF("Si4463 initialized\n");

        auto testData = std::to_array<Byte>({0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b});
        constexpr auto nTransmissions = 5;
        auto waitTime = 1 * RODOS::SECONDS;
        for(auto i = 0; i < nTransmissions; ++i)
        {
            RODOS::PRINTF("Transmitting...\n");
            periphery::rf::TransmitData(std::span<Byte>(testData));
            RODOS::AT(RODOS::NOW() + waitTime);
        }

        RODOS::PRINTF("Test data sent\n");
    }
} rfTransmitTest;
}