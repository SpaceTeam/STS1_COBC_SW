//! @file
//! @brief A program to test the GetStatus EDU command
//!
//! Preparation:
//!
//! Example:
//! D78B0A0011FFCCBB55EE99AA7722A7656CF5
//! 8B050011223344558B58B4C3
//! 59

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

        //RODOS::PRINTF("\nReturned error code: %d", static_cast<int>(resultInfo.errorCode));
        //RODOS::PRINTF("\nReturned total result size: %d", static_cast<int>(resultInfo.resultSize));
    }
};


auto const returnResultTest = ReturnResultTest();
}