//! @file
//! @brief A program to test the GetStatus EDU command
//!
//! Preparation:
//!
//! Example:
//! D78B0A0011FFCCBB55EE99AA7722911DCFA7
//! 8B05001122334455A59CFA94
//! 59 (= EOF)

#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/Periphery/Enums.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw
{
auto edu = periphery::Edu();


class ReturnResultTest : public RODOS::StaticThread<>
{
    void init() override
    {
        edu.Initialize();
    }


    void run() override
    {
        RODOS::PRINTF("\nSending command 'Return Result'\n");

        auto resultInfo = edu.ReturnResult();

        RODOS::PRINTF("\nReturned error code: %d", static_cast<int>(resultInfo.errorCode));
        RODOS::PRINTF("\nReturned total result size: %lu",
                      static_cast<unsigned long>(resultInfo.resultSize.get()));
    }
};


auto const returnResultTest = ReturnResultTest();
}