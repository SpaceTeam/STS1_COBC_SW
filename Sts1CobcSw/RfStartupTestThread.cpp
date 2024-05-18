namespace sts1cobcsw::rf
{
class RfStartupTestThread : public RODOS::StaticThread<stackSize>
{
public:
    RfStartupTestThread() : StaticThread("RfStartupTestThread", RfStartupTestThreadPriority)
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
        Initialize(TxType::morse);
        auto partNumber = ReadPartNumber();
        if(partNumber != 0x4463)
        {
            rfIsWorking = false;
        }

        // Wake up SPI startup test and supervisor thread

        // Suspend until EOT
        RODOS::AT(RODOS::END_OF_TIME);
    }
} RfStartupTestThread;
}
