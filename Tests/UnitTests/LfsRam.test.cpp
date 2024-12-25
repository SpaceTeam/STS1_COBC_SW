#include <Tests/UnitTests/UnitTestThread.hpp>

#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#include <Sts1CobcSw/FileSystem/LfsRam.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <littlefs/lfs.h>

#include <rodos_no_using_namespace.h>


auto RunUnitTest() -> void
{
    namespace fs = sts1cobcsw::fs;
    using sts1cobcsw::operator""_b;

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

        // The first time the pattern 0xC0FFEEEE is written to memory it is replaced with
        // 0xDDFFEEEE. Subsequent writes will not be replaced. This is done on the device level to
        // simulate a write/memory fault. Littlefs should detect this and automatically write to a
        // different location, so lfs_file_write() does not fail.
        fs::SimulateFailOnNextWrite();
        static constexpr auto corruptionPattern = std::array{0xC0_b, 0xFF_b, 0xEE_b, 0xEE_b};
        errorCode = lfs_file_write(&lfs, &file, corruptionPattern.data(), corruptionPattern.size());
        Require(errorCode == static_cast<int>(corruptionPattern.size()));

        errorCode = lfs_file_close(&lfs, &file);
        Require(errorCode == 0);
        errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_RDONLY);
        Require(errorCode == 0);

        // Since littlefs detected and corrected the write fault, we should read the correct data
        auto readData = decltype(corruptionPattern){};
        errorCode = lfs_file_read(&lfs, &file, readData.data(), readData.size());
        Require(errorCode == static_cast<int>(readData.size()));
        Require(readData == corruptionPattern);

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

        auto const * filePath = "/BitFlip";
        auto file = lfs_file_t{};
        errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_WRONLY | LFS_O_CREAT);
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

        // Search for the written data in memory and flip a bit
        auto it =
            std::search(fs::memory.begin(), fs::memory.end(), writeData.begin(), writeData.end());
        Require(it != fs::memory.end());
        *it ^= 0x08_b;

        if(i == 0)
        {
            errorCode = lfs_mount(&lfs, &fs::lfsConfig);
            Require(errorCode == 0);
        }
        errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_RDONLY);
        Require(errorCode == 0);

        // File can be read but it is empty
        auto readData = decltype(writeData){};
        errorCode = lfs_file_read(&lfs, &file, readData.data(), readData.size());
        Require(errorCode == 0);
        Require(readData != writeData);
        auto size = lfs_file_size(&lfs, &file);
        Require(size == 0);

        errorCode = lfs_file_close(&lfs, &file);
        Require(errorCode == 0);
        errorCode = lfs_unmount(&lfs);
        Require(errorCode == 0);
    }
}
