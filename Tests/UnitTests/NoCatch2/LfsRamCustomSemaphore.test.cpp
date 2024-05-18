#include <Sts1CobcSw/FileSystem/LfsStorageDevice.hpp>

#include <littlefs/lfs.h>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
using RODOS::PRINTF;


class LfsRamCustomSemaphoreTest : public RODOS::StaticThread<>
{
public:
    LfsRamCustomSemaphoreTest() : StaticThread("LfsRamCustomSemaphoreTest")
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
        assert(errorCode == 0);
        errorCode = lfs_mount(&lfs, &sts1cobcsw::fs::lfsConfig);
        assert(errorCode == 0);

        auto const * directoryPath = "MyFolder";
        errorCode = lfs_mkdir(&lfs, directoryPath);
        assert(errorCode == 0);

        auto const * filePath = "MyFolder/MyFile";
        auto file = lfs_file_t{};
        errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_WRONLY | LFS_O_CREAT);
        assert(errorCode == 0);

        int number = 123;
        errorCode = lfs_file_write(&lfs, &file, &number, sizeof(number));
        assert(errorCode == sizeof(number));

        errorCode = lfs_file_close(&lfs, &file);
        assert(errorCode == 0);
        errorCode = lfs_unmount(&lfs);
        assert(errorCode == 0);

        errorCode = lfs_mount(&lfs, &sts1cobcsw::fs::lfsConfig);
        assert(errorCode == 0);
        errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_RDONLY);
        assert(errorCode == 0);

        int readNumber = 0;
        errorCode = lfs_file_read(&lfs, &file, &readNumber, sizeof(number));
        assert(errorCode == sizeof(number));
        assert(readNumber == number);

        errorCode = lfs_file_close(&lfs, &file);
        assert(errorCode == 0);
        RODOS::PRINTF("Done.\n");
    }
} lfsRamCustomSemaphoreTest;
}
