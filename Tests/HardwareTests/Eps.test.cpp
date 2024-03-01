#include <Sts1CobcSw/Periphery/Eps.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
using RODOS::PRINTF;


class EpsTest : public RODOS::StaticThread<>
{
public:
    EpsTest() : StaticThread("EpsTest")
    {
    }


private:
    void init() override
    {
    }


    void run() override
    {
        PRINTF("\nEPS test\n\n");

        eps::Initialize();
        PRINTF("EPS ADCs initialized\n");

        PRINTF("\n");

        // Here comes the rest of the EPS test
    }
} epsTest;
}
