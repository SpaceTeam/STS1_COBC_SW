namespace sts1cobcsw::flash
{
class FlashStartupTestThread : public RODOS::StaticThread<stackSize>
{
public:
    FlashStartupTestThread() : StaticThread("FlashStartupTestThread", FlashStartupTestThreadPriority)
    {
    }

private:
    void init() override
    {
    }


    void run() override
    {
        //Suspend until EOT
        RODOS::AT(RODOS::END_OF_TIME);

        //Initialize device and read its ID
        Initialize();
        auto jedecId = ReadJedecId(); 
        if((jedecId.manufacturerId != 0xEF)||(jedecId.deviceId == 0x4021)){
            flashIsWorking = false;
        }

        //Wake up SPI startup test and supervisor thread

        //Suspend until EOT
        RODOS::AT(RODOS::END_OF_TIME);

    }
} FlashStartupTestThread;
}
