#include <Sts1CobcSw/FileSystem/LfsStorageDevice.hpp>

#include <littlefs/lfs.h>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
using RODOS::PRINTF;


class LockingThread : public RODOS::StaticThread<>
{
public:
    explicit LockingThread(const char * threadName) : StaticThread(threadName)
    {
    }


private:
    void init() override
    {
    }


    void run() override
    {
        sts1cobcsw::fs::Initialize();
        auto lfs = lfs_t{};
        auto errorCode = lfs_format(&lfs, &sts1cobcsw::fs::lfsConfig);
        if(errorCode == 0)
        {
            PRINTF("%s: Success\n", this->name);
        }
        else
        {
            PRINTF("%s: Error %d\n", this->name, errorCode);
        }
    }
};
auto thread1 = LockingThread("LockingThread1");
auto thread2 = LockingThread("LockingThread2");
}
