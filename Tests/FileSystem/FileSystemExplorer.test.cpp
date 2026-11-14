#include <Sts1CobcSw/FileSystem/Explorer.hpp>
#include <Sts1CobcSw/FileSystem/FileSystem.hpp>

#include <rodos/api/rodos.h>

using namespace sts1cobcsw::fs;

class ExplorerTest : public StaticThread<>
{
    void run() override
    {
        SuspendFor(2 * SECONDS);

        PrintWorkingDirectory();

        auto result = CreateDirectory("/test");
        if(not result) {
            printf("mkdir failed\n");
        }

        ListDirectory("/");

        ChangeDirectory("/test");
        PrintWorkingDirectory();

        Touch("/test/hello.txt");

        ListDirectory("/test");

        RemoveObject("/test/hello.txt");
    }
} explorerTest;