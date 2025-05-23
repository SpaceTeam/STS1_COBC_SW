#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Reports.hpp>
#include <Sts1CobcSw/RfProtocols/TmTransferFrame.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
constexpr auto stackSize = 2000U;

class ReportFramesTest : public RODOS::StaticThread<stackSize>
{
public:
    ReportFramesTest() : StaticThread("ReportFramesTest")
    {
    }


private:
    auto init() -> void override
    {
    }


    auto run() -> void override
    {
        RODOS::PRINTF("Generatirng TM Transfer Frames for all reports\n");
    }
} reportFramesTest;
}
