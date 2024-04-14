#include <Sts1CobcSw/FileSystem/FileSystem.hpp>

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
        fs::Initialize();
    }


    void run() override
    {
        PRINTF("\n");
        PRINTF("File system test\n");


        auto errorCode = fs::Mount();
        if(errorCode < 0)
        {
            fs::Format();
        }

        PRINTF("\n");
        fs::Ls("/");

        auto const * directoryPath = "MyFolder";
        PRINTF("\n");
        PRINTF("Creating directory '%s' ...\n", directoryPath);
        fs::CreateDirectory(directoryPath);
        fs::Ls("/");

        auto const * filePath = "MyFolder/MyFile";
        PRINTF("\n");
        PRINTF("Creating file '%s' ...\n", filePath);
        fs::OpenFile(filePath, LFS_O_WRONLY | LFS_O_CREAT);

        PRINTF("Writing to file ...\n");
        int number = 123;
        fs::WriteToFile(number);
        number = 456;
        fs::WriteToFile(number);
        fs::CloseFile();

        PRINTF("Reading from file ...\n");
        fs::OpenFile(filePath, LFS_O_RDONLY);
        fs::ReadFromFile(&number);
        PRINTF("  number = %d == 123\n", number);
        fs::ReadFromFile(&number);
        PRINTF("  number = %d == 456\n", number);
        fs::CloseFile();

        fs::Ls(directoryPath);

        PRINTF("\n");
        PRINTF("Deleting file '%s' ...\n", filePath);
        fs::Remove(filePath);
        fs::Ls(directoryPath);

        PRINTF("\n");
        PRINTF("Deleting directory '%s' ...\n", directoryPath);
        fs::Remove(directoryPath);
        fs::Ls("/");

        fs::Unmount();
    }
} fileSystemTest;
}
