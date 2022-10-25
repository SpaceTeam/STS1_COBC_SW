#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/Enums.hpp>

#include <stm32f4xx_crc.h>

#include <rodos_no_using_namespace.h>

#include <cstdint>

namespace sts1cobcsw
{

auto eduUart = periphery::Edu();
uint16_t programId = 0xFFFF;
uint16_t queueId = 0x0000;
uint16_t timeout = 0xFFFF;

class ExecuteProgramTest : public RODOS::StaticThread<>
{
    void init() override
    {
    }

    void run() override
    {
        // TODO(Daniel): check CRC32 byte order
        RODOS::PRINTF("START\n");
        eduUart.ExecuteProgram(programId, queueId, timeout);
        RODOS::PRINTF("DONE\n");
    }
};


auto const executeProgramTest = ExecuteProgramTest();
}