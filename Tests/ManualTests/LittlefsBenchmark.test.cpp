#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <littlefs/lfs.h>
#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/string.h>
#include <etl/to_string.h>

#include <algorithm>
#include <array>
#include <ranges>
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
        auto lfs = lfs_t{};
        auto errorCode = lfs_mount(&lfs, &fs::lfsConfig);
        if(errorCode != 0)
        {
            PRINTF("Mount failed with error code %d\n", errorCode);
            return;
        }

        // Setting up files, paths, buffers, ...
        static constexpr auto nFiles = 1;
        auto files = std::array<lfs_file_t, nFiles>{};
        auto filePaths = std::array<Path, nFiles>{};
        auto fileBuffers = std::array<std::array<Byte, fs::lfsCacheSize>, nFiles>{};
        auto lfsFileConfigs = std::array<lfs_file_config, nFiles>{};
        auto errorCodes = std::array<int, nFiles>{};
        for(auto i = 0U; i < nFiles; ++i)
        {
            filePaths[i] = Path("File");
            etl::to_string(i, filePaths[i], /*append=*/true);
            lfsFileConfigs[i] = lfs_file_config{.buffer = fileBuffers[i].data()};
        }

        PRINTF("Opening %d files ...\n", nFiles);
        auto beginOpen = CurrentRodosTime();
        for(auto i = 0U; i < nFiles; ++i)
        {
            errorCodes[i] = lfs_file_opencfg(&lfs,
                                             &files[i],
                                             filePaths[i].data(),
                                             LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC,  // NOLINT
                                             &lfsFileConfigs[i]);
        }
        auto endOpen = CurrentRodosTime();
        if(std::ranges::any_of(errorCodes, [](int error) { return error != 0; }))
        {
            PRINTF("File open failed with error code %d\n",
                   *std::ranges::find_if(errorCodes, [](int ec) { return ec != 0; }));
            return;
        }
        PRINTF("  took %5u ms\n", static_cast<unsigned int>((endOpen - beginOpen) / ms));
        PRINTF("\n");


        static constexpr auto nChunks = 50U;
        auto chunk = std::array<Byte, 200>{};
        PRINTF("Writing %u x %d B to all %d files ...\n",
               nChunks,
               static_cast<int>(chunk.size()),
               nFiles);
        errorCodes.fill(chunk.size());
        auto beginWrite = CurrentRodosTime();
        for(auto i = 0U; i < nFiles; ++i)
        {
            for(auto j = 0U; j < nChunks; ++j)
            {
                errorCodes[i] = lfs_file_write(&lfs, &files[i], chunk.data(), chunk.size());
                if(errorCodes[i] != static_cast<int>(chunk.size()))
                {
                    break;
                }
            }
        }
        auto endWrite = CurrentRodosTime();
        if(std::ranges::any_of(errorCodes, [&](int error) { return error != chunk.size(); }))
        {
            PRINTF("File write failed with error code %d\n",
                   *std::ranges::find_if(errorCodes,
                                         [&](int error) { return error != chunk.size(); }));
            return;
        }
        PRINTF("  took %5u ms\n", static_cast<unsigned int>((endWrite - beginWrite) / ms));
        PRINTF("\n");

        PRINTF("Setting file positions to 0 for all %d files ...\n", nFiles);
        auto beginSeek = CurrentRodosTime();
        for(auto i = 0U; i < nFiles; ++i)
        {
            errorCodes[i] = lfs_file_seek(&lfs, &files[i], 0, LFS_SEEK_SET);
        }
        auto endSeek = CurrentRodosTime();
        if(std::ranges::any_of(errorCodes, [](int error) { return error < 0; }))
        {
            PRINTF("File seek failed with error code %d\n",
                   *std::ranges::find_if(errorCodes, [](int ec) { return ec < 0; }));
            return;
        }
        PRINTF("  took %5u ms\n", static_cast<unsigned int>((endSeek - beginSeek) / ms));
        PRINTF("\n");

        PRINTF("Overwriting %u x %d B in all %d files ...\n",
               nChunks,
               static_cast<int>(chunk.size()),
               nFiles);
        chunk.fill(Byte{0x17});
        errorCodes.fill(chunk.size());
        auto beginOverwrite = CurrentRodosTime();
        for(auto i = 0U; i < nFiles; ++i)
        {
            for(auto j = 0U; j < nChunks; ++j)
            {
                errorCodes[i] = lfs_file_write(&lfs, &files[i], chunk.data(), chunk.size());
                if(errorCodes[i] != static_cast<int>(chunk.size()))
                {
                    break;
                }
            }
        }
        auto endOverwrite = CurrentRodosTime();
        if(std::ranges::any_of(errorCodes, [&](int error) { return error != chunk.size(); }))
        {
            PRINTF("File overwrite failed with error code %d\n",
                   *std::ranges::find_if(errorCodes,
                                         [&](int error) { return error != chunk.size(); }));
            return;
        }
        PRINTF("  took %5u ms\n", static_cast<unsigned int>((endOverwrite - beginOverwrite) / ms));
        PRINTF("\n");

        PRINTF("Closing all %d files ...\n", nFiles);
        auto beginClose = CurrentRodosTime();
        for(auto i = 0U; i < nFiles; ++i)
        {
            errorCodes[i] = lfs_file_close(&lfs, &files[i]);
        }
        auto endClose = CurrentRodosTime();
        if(std::ranges::any_of(errorCodes, [](int error) { return error != 0; }))
        {
            PRINTF("File close failed with error code %d\n",
                   *std::ranges::find_if(errorCodes, [](int ec) { return ec != 0; }));
            return;
        }
        PRINTF("  took %5u ms\n", static_cast<unsigned int>((endClose - beginClose) / ms));
        PRINTF("\n");

        PRINTF("Unmounting ...");
        lfs_unmount(&lfs);
        PRINTF(" done\n");
    }
} littlefsBenchmarkThread;
}
}
