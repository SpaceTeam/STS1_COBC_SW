#include <Tests/UnitTests/UnitTestThread.hpp>

#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>

#include <littlefs/lfs.h>


auto RunUnitTest() -> void
{
    sts1cobcsw::fs::Initialize();
    auto lfs = lfs_t{};
    auto errorCode = lfs_format(&lfs, &sts1cobcsw::fs::lfsConfig);
    Require(errorCode == 0);
    errorCode = lfs_mount(&lfs, &sts1cobcsw::fs::lfsConfig);
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

    errorCode = lfs_mount(&lfs, &sts1cobcsw::fs::lfsConfig);
    Require(errorCode == 0);
    errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_RDONLY);
    Require(errorCode == 0);

    int readNumber = 0;
    errorCode = lfs_file_read(&lfs, &file, &readNumber, sizeof(number));
    Require(errorCode == sizeof(number));
    Require(readNumber == number);

    errorCode = lfs_file_close(&lfs, &file);
    Require(errorCode == 0);
}
