#include <Sts1CobcSw/FileSystem/FileSystem.hpp>

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
        fs::Mount();
    }


    void run() override
    {
        fs::Unmount();
    }
} fileSystemTest;
}
