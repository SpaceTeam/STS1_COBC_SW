#include <Sts1CobcSw/FileSystem/FileSystem.hpp>

#include <littlefs/lfs.h>

#include <rodos_no_using_namespace.h>

#include <array>
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
        fs::Mount();

        fs::OpenFile("MyFile", LFS_O_RDWR | LFS_O_CREAT);

        auto fileSize = fs::FileSize();
        PRINTF("\nFile size = %d == 12\n", fileSize);

        int number = 0;
        fs::ReadFromFile(&number);
        PRINTF("\n");
        PRINTF("Number = %d == 123\n", number);

        fs::ReadFromFile(&number);
        PRINTF("\n");
        PRINTF("Number = %d == 12345\n", number);

        fs::ReadFromFile(&number);
        PRINTF("\n");
        PRINTF("Number = %d == 1234567\n", number);

        // number = 1234567;
        // fs::WriteToFile(number);
        // lfs_file_write(&fs::lfs, &fs::lfsFile, &number, sizeof(number));

        fs::CloseFile();

        fs::Ls("/");

        fs::Unmount();
    }
} fileSystemTest;
}
