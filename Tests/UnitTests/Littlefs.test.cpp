#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#ifdef __linux__
    #include <Sts1CobcSw/FileSystem/LfsRam.hpp>
#endif
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <littlefs/lfs.h>

#include <etl/string.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <vector>


namespace fs = sts1cobcsw::fs;

using sts1cobcsw::operator""_b;
using Path = etl::string<fs::maxPathLength>;


TEST_CASE("Littlefs without data corruption")
{
    fs::Initialize();
    auto lfs = lfs_t{};
    auto errorCode = lfs_format(&lfs, &fs::lfsConfig);
    CHECK(errorCode == 0);
    errorCode = lfs_mount(&lfs, &fs::lfsConfig);
    REQUIRE(errorCode == 0);

    auto const * directoryPath = "MyFolder";
    errorCode = lfs_mkdir(&lfs, directoryPath);
    CHECK(errorCode == 0);
    auto filePath = Path(directoryPath);
    filePath.append("/MyFile");
    auto file = lfs_file_t{};
    errorCode = lfs_file_open(&lfs, &file, filePath.c_str(), LFS_O_WRONLY | LFS_O_CREAT);
    CHECK(errorCode == 0);
    static constexpr auto writtenNumber = 123;
    errorCode = lfs_file_write(&lfs, &file, &writtenNumber, sizeof(writtenNumber));
    CHECK(errorCode == static_cast<int>(sizeof(writtenNumber)));
    errorCode = lfs_file_close(&lfs, &file);
    CHECK(errorCode == 0);

    errorCode = lfs_unmount(&lfs);
    CHECK(errorCode == 0);
    errorCode = lfs_mount(&lfs, &fs::lfsConfig);
    CHECK(errorCode == 0);

    file = lfs_file_t{};
    errorCode = lfs_file_open(&lfs, &file, filePath.c_str(), LFS_O_RDONLY);
    CHECK(errorCode == 0);
    auto readNumber = 0;
    errorCode = lfs_file_read(&lfs, &file, &readNumber, sizeof(readNumber));
    CHECK(errorCode == static_cast<int>(sizeof(readNumber)));
    CHECK(readNumber == writtenNumber);
    errorCode = lfs_file_close(&lfs, &file);
    CHECK(errorCode == 0);

    errorCode = lfs_remove(&lfs, directoryPath);
    CHECK(errorCode == LFS_ERR_NOTEMPTY);
    errorCode = lfs_remove(&lfs, "NonExistentFolder/");
    CHECK(errorCode == LFS_ERR_NOENT);
    errorCode = lfs_remove(&lfs, "NonExistentFile.txt");
    CHECK(errorCode == LFS_ERR_NOENT);
    errorCode = lfs_remove(&lfs, filePath.c_str());
    CHECK(errorCode == 0);
    errorCode = lfs_remove(&lfs, directoryPath);
    CHECK(errorCode == 0);

    errorCode = lfs_unmount(&lfs);
    CHECK(errorCode == 0);
}


#ifdef __linux__
auto TryToCorruptDataInMemory(std::span<const sts1cobcsw::Byte> dataToCorrupt) -> bool;


TEST_CASE("Littlefs with data corruption")
{
    fs::Initialize();
    auto lfs = lfs_t{};
    auto errorCode = lfs_format(&lfs, &fs::lfsConfig);
    CHECK(errorCode == 0);

    // Faulty write
    {
        errorCode = lfs_mount(&lfs, &fs::lfsConfig);
        CHECK(errorCode == 0);

        auto const * filePath = "/FaultyWrite";
        auto file = lfs_file_t{};
        errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_WRONLY | LFS_O_CREAT);
        CHECK(errorCode == 0);

        // The first time 0xC0FFEEEE is written to memory it is corrupted (a bit is flipped).
        // Subsequent writes will not be replaced. This simulates a write/memory fault. Littlefs
        // should detect this and automatically write to a different location, so lfs_file_write()
        // does not fail.
        static constexpr auto writeData = std::array{0xC0_b, 0xFF_b, 0xEE_b, 0xEE_b};
        fs::SetProgramFinishedHandler(
            []()
            {
                static auto corruptNextWrite = true;
                if(corruptNextWrite)
                {
                    auto dataWasFoundAndCorrupted = TryToCorruptDataInMemory(writeData);
                    corruptNextWrite = not dataWasFoundAndCorrupted;
                }
            });
        errorCode = lfs_file_write(&lfs, &file, writeData.data(), writeData.size());
        CHECK(errorCode == static_cast<int>(writeData.size()));

        errorCode = lfs_file_close(&lfs, &file);
        CHECK(errorCode == 0);
        // Reset the program finished handler after closing the file because that is when the data
        // is really written to memory
        fs::SetProgramFinishedHandler(nullptr);
        errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_RDONLY);
        CHECK(errorCode == 0);

        // Check that both 0xC0FFEEEE and 0x40FFEEEE are in memory
        auto it =
            std::search(fs::memory.begin(), fs::memory.end(), writeData.begin(), writeData.end());
        CHECK(it != fs::memory.end());
        auto corruptedData = std::array{0x40_b, 0xFF_b, 0xEE_b, 0xEE_b};
        it = std::search(
            fs::memory.begin(), fs::memory.end(), corruptedData.begin(), corruptedData.end());
        CHECK(it != fs::memory.end());

        // Since littlefs detected and corrected the write fault, we should read the correct data
        auto readData = decltype(writeData){};
        errorCode = lfs_file_read(&lfs, &file, readData.data(), readData.size());
        CHECK(errorCode == static_cast<int>(readData.size()));
        CHECK(readData == writeData);

        errorCode = lfs_file_close(&lfs, &file);
        CHECK(errorCode == 0);
        errorCode = lfs_unmount(&lfs);
        CHECK(errorCode == 0);
    }

    // Bit flip
    // - i = 0: bit flip while unmounted with small file (<= read size)
    // - i = 1: bit flip while mounted with small file (<= read size)
    // - i = 2: bit flip while unmounted with large file (> read size)
    // - i = 3: bit flip while mounted with large file (> read size)
    for(auto i = 0; i < 4; ++i)
    {
        errorCode = lfs_mount(&lfs, &fs::lfsConfig);
        CHECK(errorCode == 0);

        static auto const filePath = "/BitFlip" + std::to_string(i);
        auto file = lfs_file_t{};
        errorCode = lfs_file_open(&lfs, &file, filePath.c_str(), LFS_O_WRONLY | LFS_O_CREAT);
        CHECK(errorCode == 0);

        static constexpr auto dataToCorrupt = std::array{0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b};
        auto fileSize = i < 2 ? 252U : 257U;
        auto writeData = std::vector(fileSize, 0xB0_b);
        std::copy(dataToCorrupt.begin(), dataToCorrupt.end(), writeData.begin());
        errorCode = lfs_file_write(
            &lfs, &file, writeData.data(), static_cast<lfs_size_t>(writeData.size()));
        CHECK(errorCode == static_cast<int>(writeData.size()));

        errorCode = lfs_file_close(&lfs, &file);
        CHECK(errorCode == 0);
        if(i % 2 == 0)
        {
            errorCode = lfs_unmount(&lfs);
            CHECK(errorCode == 0);
        }
        TryToCorruptDataInMemory(dataToCorrupt);
        if(i % 2 == 0)
        {
            errorCode = lfs_mount(&lfs, &fs::lfsConfig);
            CHECK(errorCode == 0);
        }
        errorCode = lfs_file_open(&lfs, &file, filePath.c_str(), LFS_O_RDONLY);
        CHECK(errorCode == 0);

        // If the original file size is <= 252 bytes (read size), the corrupted file size is 0.
        // Otherwise, the file size does not change.
        auto size = lfs_file_size(&lfs, &file);
        if(i < 2)
        {
            CHECK(size == 0);
        }
        else
        {
            CHECK(size == static_cast<int>(fileSize));
        }

        // Reading never fails, but it reads either 0 bytes or the corrupted data
        auto readData = std::vector<sts1cobcsw::Byte>(writeData.size());
        errorCode =
            lfs_file_read(&lfs, &file, readData.data(), static_cast<lfs_size_t>(readData.size()));
        if(i < 2)
        {
            CHECK(errorCode == 0);
        }
        else
        {
            CHECK(errorCode == LFS_ERR_CORRUPT);
        }
        CHECK(readData != writeData);

        errorCode = lfs_file_close(&lfs, &file);
        CHECK(errorCode == 0);
        errorCode = lfs_unmount(&lfs);
        CHECK(errorCode == 0);
    }
}


auto TryToCorruptDataInMemory(std::span<sts1cobcsw::Byte const> dataToCorrupt) -> bool
{
    auto it = std::search(
        fs::memory.begin(), fs::memory.end(), dataToCorrupt.begin(), dataToCorrupt.end());
    if(it == fs::memory.end())
    {
        return false;
    }
    *it ^= 0x80_b;
    return true;
}
#endif
