namespace sts1cobcsw
{
class FramEpsStartupTestThread : public RODOS::StaticThread<stackSize>
{
public:
    FramEpsStartupTestThread()
        : StaticThread("FramEpsStartupTestThread", FramEpsStartupTestThreadPriority)
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

        // Initialize devices and read its ID
        fram::Initialize();
        auto deviceId = fram::ReadDeviceId();
        auto correctDeviceId =
            std::to_array({0x03_b, 0x2E_b, 0xC2_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b});
        if(deviceID != correctDeviceId)
        {
            framIsWorking = false;
        }

        eps::Initialize();

        // Wake up SPI startup test and supervisor thread

        // Suspend until EOT
        RODOS::AT(RODOS::END_OF_TIME);
    }
} FramEpsStartupTestThread;
}
