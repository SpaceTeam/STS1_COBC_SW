#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/ThreadPriorities.hpp>


namespace sts1cobcsw
{
constexpr auto stackSize = 100U;


class FlashStartupTestThread : public RODOS::StaticThread<stackSize>
{
public:
    FlashStartupTestThread()
        : StaticThread("FlashStartupTestThread", flashStartupTestThreadPriority)
    {
    }

private:
    void init() override
    {
    }


    void run() override
    {
        // Suspend until EOT
        RODOS::AT(RODOS::END_OF_TIME);

        // Initialize device and read its ID
        flash::Initialize();
        auto jedecId = flash::ReadJedecId();
        if((jedecId.manufacturerId != 0xEF) || (jedecId.deviceId != 0x4021))
        {
            flash::flashIsWorking = false;
        }

        // Wake up SPI startup test and supervisor thread

        // Suspend until EOT
        RODOS::AT(RODOS::END_OF_TIME);
    }
} FlashStartupTestThread;
}
