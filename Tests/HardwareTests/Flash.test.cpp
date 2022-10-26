#include <Sts1CobcSw/Periphery/Flash.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
using RODOS::PRINTF;


class FlashTest : public RODOS::StaticThread<>
{
    void init() override
    {
        auto errorCode = sts1cobcsw::periphery::flash::Initialize();
        PRINTF("Initialize() returned %i", static_cast<int>(errorCode));
    }


    void run() override
    {
    }
};


const auto flashTest = FlashTest();
}