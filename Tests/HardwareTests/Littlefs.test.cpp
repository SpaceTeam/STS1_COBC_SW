#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>
#include <Tests/HardwareTests/Utility.hpp>

#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>

#include <littlefs/lfs.h>

#include <rodos_no_using_namespace.h>

#include <cstddef>


namespace sts1cobcsw
{
using RODOS::PRINTF;


// The minimum required stack size is ~2540 bytes
constexpr std::size_t stackSize = 2600;


class LittlefsTest : public RODOS::StaticThread<stackSize>
{
public:
    LittlefsTest() : StaticThread("LittlefsTest")
    {
    }


private:
    auto init() -> void override
    {
        InitializeRfLatchupDisablePins();
        fs::Initialize();
    }


    auto run() -> void override
    {
        EnableRfLatchupProtection();

        PRINTF("\n\n");
        PRINTF("littlefs test\n");

        PRINTF("\n");
        auto lfs = lfs_t{};
        PRINTF("Formatting ...\n");
        auto errorCode = lfs_format(&lfs, &fs::lfsConfig);
        Check(errorCode == 0);
        PRINTF("Mounting ...\n");
        errorCode = lfs_mount(&lfs, &fs::lfsConfig);
        Check(errorCode == 0);

        PRINTF("\n");
        auto const * directoryPath = "MyFolder";
        PRINTF("Creating directory '%s' ...\n", directoryPath);
        errorCode = lfs_mkdir(&lfs, directoryPath);
        Check(errorCode == 0);

        PRINTF("\n");
        auto const * filePath = "MyFolder/MyFile";
        PRINTF("Creating file '%s' ...\n", filePath);
        auto file = lfs_file_t{};
        errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_WRONLY | LFS_O_CREAT);
        Check(errorCode == 0);
        int number = 123;
        PRINTF("Writing %d to file ...\n", number);
        errorCode = lfs_file_write(&lfs, &file, &number, sizeof(number));
        Check(errorCode == sizeof(number));
        PRINTF("Closing file ...\n");
        errorCode = lfs_file_close(&lfs, &file);
        Check(errorCode == 0);

        PRINTF("\n");
        PRINTF("Unmounting ...\n");
        errorCode = lfs_unmount(&lfs);
        Check(errorCode == 0);
        PRINTF("Mounting again ...\n");
        errorCode = lfs_mount(&lfs, &fs::lfsConfig);
        Check(errorCode == 0);

        PRINTF("\n");
        PRINTF("Opening file '%s' for reading ...\n", filePath);
        errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_RDONLY);
        Check(errorCode == 0);
        int readNumber = 0;
        PRINTF("Reading from file ...\n");
        errorCode = lfs_file_read(&lfs, &file, &readNumber, sizeof(number));
        Check(errorCode == sizeof(number));
        PRINTF("Read number = %d == %d\n", readNumber, number);
        Check(readNumber == number);
        PRINTF("Closing file ...\n");
        errorCode = lfs_file_close(&lfs, &file);
        Check(errorCode == 0);
    }
} littlefsTest;
}
