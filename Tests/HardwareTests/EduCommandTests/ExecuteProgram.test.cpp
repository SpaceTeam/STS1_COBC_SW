#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/Periphery/EduEnums.hpp>

#include <stm32f4xx_crc.h>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw
{
auto edu = periphery::Edu();


class ExecuteProgramTest : public RODOS::StaticThread<>
{
    void init() override
    {
        edu.Initialize();
    }


    void run() override
    {
        constexpr std::uint16_t programId = 0;
        constexpr std::uint16_t queueId = 1;
        constexpr std::int16_t timeout = 5;

        RODOS::PRINTF("\nSending command 'Execute Program' with\n");
        RODOS::PRINTF("- program ID = %04x\n", programId);
        RODOS::PRINTF("- queue ID   = %04x\n", queueId);
        RODOS::PRINTF("- timeout    = %04x\n", timeout);

        // TODO: (Daniel) check CRC32 byte order
        auto errorCode =
            edu.ExecuteProgram({.programId = programId, .queueId = queueId, .timeout = timeout});
        RODOS::PRINTF("\nReturned error code: %d", static_cast<int>(errorCode));
    }
};


auto const executeProgramTest = ExecuteProgramTest();
}