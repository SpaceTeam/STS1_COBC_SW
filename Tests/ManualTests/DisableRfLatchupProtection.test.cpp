#include <Tests/HardwareSetup/RfLatchupProtection.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
class DisableRfLatchupProtectionTest : public RODOS::StaticThread<>
{
    void init() override
    {
    }


    void run() override
    {
        DisableRfLatchupProtection();
    }
} disableRfLatchupProtectionTest;
}
