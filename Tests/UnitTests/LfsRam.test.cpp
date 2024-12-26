#include <Tests/UnitTests/UnitTestThread.hpp>

#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#include <Sts1CobcSw/FileSystem/LfsRam.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <littlefs/lfs.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <vector>


namespace fs = sts1cobcsw::fs;
using sts1cobcsw::operator""_b;


auto TryToCorruptDataInMemory(std::span<const sts1cobcsw::Byte> dataToCorrupt) -> bool;


auto RunUnitTest() -> void
{
    fs::Initialize();
    auto lfs = lfs_t{};
    auto errorCode = lfs_format(&lfs, &fs::lfsConfig);
    Require(errorCode == 0);

    // Test without memory corruption or faults
    {
        errorCode = lfs_mount(&lfs, &fs::lfsConfig);
        Require(errorCode == 0);

        auto const * directoryPath = "MyFolder";
        errorCode = lfs_mkdir(&lfs, directoryPath);
        Require(errorCode == 0);

        auto const * filePath = "MyFolder/MyFile";
        auto file = lfs_file_t{};
        errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_WRONLY | LFS_O_CREAT);
        Require(errorCode == 0);

        int number = 123;
        errorCode = lfs_file_write(&lfs, &file, &number, sizeof(number));
        Require(errorCode == sizeof(number));

        errorCode = lfs_file_close(&lfs, &file);
        Require(errorCode == 0);
        errorCode = lfs_unmount(&lfs);
        Require(errorCode == 0);

        errorCode = lfs_mount(&lfs, &fs::lfsConfig);
        Require(errorCode == 0);
        errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_RDONLY);
        Require(errorCode == 0);

        int readNumber = 0;
        errorCode = lfs_file_read(&lfs, &file, &readNumber, sizeof(number));
        Require(errorCode == sizeof(number));
        Require(readNumber == number);

        errorCode = lfs_file_close(&lfs, &file);
        Require(errorCode == 0);
        errorCode = lfs_unmount(&lfs);
        Require(errorCode == 0);
    }

    // Test with faulty write
    {
        errorCode = lfs_mount(&lfs, &fs::lfsConfig);
        Require(errorCode == 0);

        auto const * filePath = "/FaultyWrite";
        auto file = lfs_file_t{};
        errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_WRONLY | LFS_O_CREAT);
        Require(errorCode == 0);

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
        Require(errorCode == static_cast<int>(writeData.size()));

        errorCode = lfs_file_close(&lfs, &file);
        Require(errorCode == 0);
        // Reset the program finished handler after closing the file because that is when the data
        // is really written to memory
        fs::SetProgramFinishedHandler(nullptr);
        errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_RDONLY);
        Require(errorCode == 0);

        // Check that both 0xC0FFEEEE and 0x40FFEEEE are in memory
        auto it =
            std::search(fs::memory.begin(), fs::memory.end(), writeData.begin(), writeData.end());
        Require(it != fs::memory.end());
        auto corruptedData = std::array{0x40_b, 0xFF_b, 0xEE_b, 0xEE_b};
        it = std::search(
            fs::memory.begin(), fs::memory.end(), corruptedData.begin(), corruptedData.end());
        Require(it != fs::memory.end());

        // Since littlefs detected and corrected the write fault, we should read the correct data
        auto readData = decltype(writeData){};
        errorCode = lfs_file_read(&lfs, &file, readData.data(), readData.size());
        Require(errorCode == static_cast<int>(readData.size()));
        Require(readData == writeData);

        errorCode = lfs_file_close(&lfs, &file);
        Require(errorCode == 0);
        errorCode = lfs_unmount(&lfs);
        Require(errorCode == 0);
    }

    // Test with bit flip (once while mounted and once while unmounted)
    for(auto i = 0; i < 2; ++i)
    {
        errorCode = lfs_mount(&lfs, &fs::lfsConfig);
        Require(errorCode == 0);

        static auto const filePath = "/BitFlip" + std::to_string(i);
        auto file = lfs_file_t{};
        errorCode = lfs_file_open(&lfs, &file, filePath.c_str(), LFS_O_WRONLY | LFS_O_CREAT);
        Require(errorCode == 0);

        auto writeData = std::array{0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b};
        errorCode = lfs_file_write(&lfs, &file, writeData.data(), writeData.size());
        Require(errorCode == static_cast<int>(writeData.size()));

        errorCode = lfs_file_close(&lfs, &file);
        Require(errorCode == 0);
        if(i == 0)
        {
            errorCode = lfs_unmount(&lfs);
            Require(errorCode == 0);
        }
        TryToCorruptDataInMemory(writeData);
        if(i == 0)
        {
            errorCode = lfs_mount(&lfs, &fs::lfsConfig);
            Require(errorCode == 0);
        }
        errorCode = lfs_file_open(&lfs, &file, filePath.c_str(), LFS_O_RDONLY);
        Require(errorCode == 0);

        // File is empty, but it can be read (i.e. lfs_file_read() does not fail)
        auto size = lfs_file_size(&lfs, &file);
        Require(size == 0);
        auto readData = decltype(writeData){};
        errorCode = lfs_file_read(&lfs, &file, readData.data(), readData.size());
        Require(errorCode == 0);
        Require(readData != writeData);

        errorCode = lfs_file_close(&lfs, &file);
        Require(errorCode == 0);
        errorCode = lfs_unmount(&lfs);
        Require(errorCode == 0);
    }
}


auto TryToCorruptDataInMemory(std::span<const sts1cobcsw::Byte> dataToCorrupt) -> bool
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
