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

        lfs_file_open(&fs::lfs, &fs::lfsFile, "MyFile", LFS_O_RDWR | LFS_O_CREAT);

        auto fileSize = lfs_file_size(&fs::lfs, &fs::lfsFile);
        PRINTF("\nFile size = %d == 8\n", static_cast<int>(fileSize));

        int number = 0;
        lfs_file_read(&fs::lfs, &fs::lfsFile, &number, sizeof(number));
        PRINTF("\nNumber = %d == 123\n", number);

        lfs_file_read(&fs::lfs, &fs::lfsFile, &number, sizeof(number));
        PRINTF("\nNumber = %d == 12345\n", number);

        // number = 12345;
        // lfs_file_write(&fs::lfs, &fs::lfsFile, &number, sizeof(number));

        lfs_file_close(&fs::lfs, &fs::lfsFile);

        fs::Unmount();
    }
} fileSystemTest;
}
