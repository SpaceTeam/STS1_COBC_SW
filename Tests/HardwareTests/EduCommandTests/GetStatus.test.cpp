//! @file
//! @brief A program to test the GetStatus EDU command
//!
//! Preparation:
//!     - Connect the UCI UART to a computer to use with HTERM, Putty, etc.
//!     - Either make sure the EDU is functional and connected, or
//!     - Set the EDU UART to be the UCI UART (in Edu.cpp) and send the responses manually via HTERM
//!     - A successful example for each status type is provided below, if you want to test error
//!     handling,
//!       just get some value, like the length, intentionally wrong and check the error code.
//!
//! If the EDU is running and functional, there is nothing left to do after flashing.
//! If the test is done with a manual response, send one of the hex strings provided below after
//! flashing. You will have a couple of seconds after flashing, since there is an allowed timeout.
//!
//! Example responses:
//! No event:         D78B010000B4BF084E
//! -> Status Type: 0, Error Code: 1, Values all 0
//!
//! Program finished: D78B0600010A0005000CE9FC17C1
//! -> Status Type: 1, Error Code: 1, Program ID: 10 = 0xA, Queue ID: 5, Exit Code: 12 = 0xC
//!
//! Results ready:    D78B0500020A000B00D4D85968
//! -> Status Type: 2, Error Code: 1, Program ID: 10 = 0xA, Queue ID: 11 = 0xB
//!
//! Check with the EDU PDD to see which bytes correspond to which value.
//! Keep in mind that the first byte (D7) is just the initial ACK, DATA is followed by the data
//! length (2 bytes), and there is a 4 byte checksum after the data part.

#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/Periphery/EduEnums.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw
{
auto edu = periphery::Edu();


class GetStatusTest : public RODOS::StaticThread<>
{
    void init() override
    {
        edu.Initialize();
    }


    void run() override
    {
        RODOS::PRINTF("\nSending command 'Get Status'\n");

        auto status = edu.GetStatus();

        RODOS::PRINTF("\nReturned status type: %d", static_cast<int>(status.statusType));
        RODOS::PRINTF("\nReturned program ID: %d", static_cast<int>(status.programId));
        RODOS::PRINTF("\nReturned queue ID: %d", static_cast<int>(status.queueId));
        RODOS::PRINTF("\nReturned exit code: %d", static_cast<int>(status.exitCode));
        RODOS::PRINTF("\nReturned error code: %d", static_cast<int>(status.errorCode));
    }
};


auto const getStatusTest = GetStatusTest();
}