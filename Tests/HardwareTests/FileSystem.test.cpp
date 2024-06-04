#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>

#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>

#include <littlefs/lfs.h>

#include <rodos_no_using_namespace.h>

#include <cstddef>


namespace sts1cobcsw
{
using RODOS::PRINTF;


constexpr std::size_t stackSize = 5'000;


class FileSystemTest : public RODOS::StaticThread<stackSize>
{
public:
    FileSystemTest() : StaticThread("FileSystemTest")
    {
    }

private:
    void init() override
    {
        fs::deprecated::Initialize();
    }


    void run() override
    {
#if HW_VERSION >= 27
        rfLatchupDisableGpioPin.Reset();
#endif

        PRINTF("\n");
        PRINTF("File system test\n");

        auto errorCode = fs::deprecated::Mount();
        if(errorCode < 0)
        {
            fs::deprecated::Format();
        }

        PRINTF("\n");
        fs::deprecated::Ls("/");

        auto const * directoryPath = "MyFolder";
        PRINTF("\n");
        PRINTF("Creating directory '%s' ...\n", directoryPath);
        fs::deprecated::CreateDirectory(directoryPath);
        fs::deprecated::Ls("/");

        auto const * filePath = "MyFolder/MyFile";
        PRINTF("\n");
        PRINTF("Creating file '%s' ...\n", filePath);
        fs::deprecated::OpenFile(filePath, LFS_O_WRONLY | LFS_O_CREAT);

        PRINTF("Writing to file ...\n");
        int number = 123;
        fs::deprecated::WriteToFile(number);
        number = 456;
        fs::deprecated::WriteToFile(number);
        fs::deprecated::CloseFile();

        PRINTF("Reading from file ...\n");
        fs::deprecated::OpenFile(filePath, LFS_O_RDONLY);
        fs::deprecated::ReadFromFile(&number);
        PRINTF("  number = %d == 123\n", number);
        fs::deprecated::ReadFromFile(&number);
        PRINTF("  number = %d == 456\n", number);
        fs::deprecated::CloseFile();

        fs::deprecated::Ls(directoryPath);

        PRINTF("\n");
        PRINTF("Deleting file '%s' ...\n", filePath);
        fs::deprecated::Remove(filePath);
        fs::deprecated::Ls(directoryPath);

        PRINTF("\n");
        PRINTF("Deleting directory '%s' ...\n", directoryPath);
        fs::deprecated::Remove(directoryPath);
        fs::deprecated::Ls("/");

        fs::deprecated::Unmount();
    }
} fileSystemTest;
}
