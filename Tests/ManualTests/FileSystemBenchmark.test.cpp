// IWYU pragma: no_include <littlefs/lfs.h>
#include <Sts1CobcSw/FileSystem/File.hpp>
#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/string.h>

#include <array>
#include <utility>


namespace sts1cobcsw
{
namespace
{
using RODOS::PRINTF;
using Path = etl::string<10>;


constexpr auto stackSize = 50'000;


class LittlefsBenchmarkThread : public RODOS::StaticThread<stackSize>
{
public:
    LittlefsBenchmarkThread() : StaticThread("LittlefsBenchmarkThread")
    {}


private:
    auto run() -> void override
    {
        PRINTF("\n");
        PRINTF("Littlefs benchmark\n");
        PRINTF("\n");
        fs::Initialize();
        fram::Initialize();
        persistentVariables.Store<"flashIsWorking">(true);

        auto result = []() -> Result<void>
        {
            OUTCOME_TRY(fs::Mount());

            PRINTF("Opening a file ...\n");
            auto beginOpen = CurrentRodosTime();
            // NOLINTNEXTLINE(*signed-bitwise)
            OUTCOME_TRY(auto file, fs::Open("MyFile", LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC));
            auto endOpen = CurrentRodosTime();
            PRINTF("  took %5u ms\n", static_cast<unsigned int>((endOpen - beginOpen) / ms));
            PRINTF("\n");

            static constexpr auto nChunks = 100U;
            auto chunk = std::array<Byte, 196>{};
            PRINTF("Writing %u x %d B to it and resizing it every time ...\n",
                   nChunks,
                   static_cast<int>(chunk.size()));
            auto beginWrite = CurrentRodosTime();
            for(auto j = 0U; j < nChunks; ++j)
            {
                OUTCOME_TRY(file.Resize((j + 1) * chunk.size()));
                OUTCOME_TRY(file.Write(Span(chunk)));
            }
            auto endWrite = CurrentRodosTime();
            PRINTF("  took %5u ms\n", static_cast<unsigned int>((endWrite - beginWrite) / ms));
            PRINTF("\n");

            static constexpr auto nResizes = 100U;
            PRINTF("Resizing it %d times ...\n", nResizes);
            auto beginResize = CurrentRodosTime();
            for(auto i = 0U; i < nResizes; ++i)
            {
                auto newSize = (nChunks + i) * chunk.size();
                OUTCOME_TRY(file.Resize(newSize));
            }
            auto endResize = CurrentRodosTime();
            PRINTF("  took %5u ms\n", static_cast<unsigned int>((endResize - beginResize) / ms));
            PRINTF("\n");

            PRINTF("Resizing the file from 0 to 10 kiB ...\n");
            OUTCOME_TRY(file.Resize(0));
            beginResize = CurrentRodosTime();
            OUTCOME_TRY(file.Resize(10 * 1024));
            endResize = CurrentRodosTime();
            PRINTF("  took %5u ms\n", static_cast<unsigned int>((endResize - beginResize) / ms));
            PRINTF("\n");

            PRINTF("Resizing the file from 0 to 100 kiB ...\n");
            OUTCOME_TRY(file.Resize(0));
            beginResize = CurrentRodosTime();
            OUTCOME_TRY(file.Resize(100 * 1024));
            endResize = CurrentRodosTime();
            PRINTF("  took %5u ms\n", static_cast<unsigned int>((endResize - beginResize) / ms));
            PRINTF("\n");

            PRINTF("Resizing the file from 0 to 1 MiB ...\n");
            OUTCOME_TRY(file.Resize(0));
            beginResize = CurrentRodosTime();
            OUTCOME_TRY(file.Resize(1024 * 1024));
            endResize = CurrentRodosTime();
            PRINTF("  took %5u ms\n", static_cast<unsigned int>((endResize - beginResize) / ms));
            PRINTF("\n");

            PRINTF("Setting its position to 0  ...\n");
            auto beginSeek = CurrentRodosTime();
            OUTCOME_TRY(file.SeekAbsolute(0));
            auto endSeek = CurrentRodosTime();
            PRINTF("  took %5u ms\n", static_cast<unsigned int>((endSeek - beginSeek) / ms));
            PRINTF("\n");

            PRINTF("Overwriting %u x %d B ...\n", nChunks, static_cast<int>(chunk.size()));
            chunk.fill(Byte{0x17});
            auto beginOverwrite = CurrentRodosTime();
            for(auto j = 0U; j < nChunks; ++j)
            {
                OUTCOME_TRY(file.Write(Span(chunk)));
            }
            auto endOverwrite = CurrentRodosTime();
            PRINTF("  took %5u ms\n",
                   static_cast<unsigned int>((endOverwrite - beginOverwrite) / ms));
            PRINTF("\n");

            PRINTF("Closing the file ...\n");
            auto beginClose = CurrentRodosTime();
            OUTCOME_TRY(file.Close());
            auto endClose = CurrentRodosTime();
            PRINTF("  took %5u ms\n", static_cast<unsigned int>((endClose - beginClose) / ms));
            PRINTF("\n");

            PRINTF("Unmounting ...");
            OUTCOME_TRY(fs::Unmount());
            PRINTF(" done\n");
            return outcome_v2::success();
        }();
        if(result.has_error())
        {
            PRINTF("Littlefs benchmark failed with error: %s\n", ToCZString(result.error()));
        }
    }
} littlefsBenchmarkThread;
}
}
