#include <Sts1CobcSw/FileSystem/ErrorsAndResult.hpp>
#include <Sts1CobcSw/FileSystem/LfsStorageDevice.hpp>
#include <Sts1CobcSw/FileSystem/LfsWrapper.hpp>

// #include <catch2/catch_test_macros.hpp>

#include <littlefs/lfs.h>

#include <iostream>


// TEST_CASE("LfsWrapper")
auto main() -> int
{
    // CHECK(true);
    sts1cobcsw::fs::Initialize();
    auto mountResult = sts1cobcsw::fs::Mount();
    // CHECK(not mountResult.has_error());
    std::cout << "mountResult.has_error() = " << mountResult.has_error() << "\n";

    auto const * filePath = "/MyFile";
    auto openResult = sts1cobcsw::fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
    // CHECK(openResult.has_value());
    std::cout << "openResult.has_error() =  " << openResult.has_error() << "\n";

    // std::cout << "File opened\n";

    // auto lfs = lfs_t{};
    // auto errorCode = lfs_format(&lfs, &sts1cobcsw::fs::lfsConfig);
    // CHECK(errorCode == 0);

    // auto result = sts1cobcsw::fs::Mount();
    // CHECK(result);

    // auto const * filePath = "MyFile";
    // auto resultFileOpen = sts1cobcsw::fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
    // CHECK(resultFileOpen);
    // sts1cobcsw::fs::File & file = resultFileOpen.value();
    // result = file.Close();
    // CHECK(result);

    // int number = 123;
    // auto resultWrite = file.Write(number);
    // CHECK(resultWrite);

    // auto resultSize = file.Size();
    // CHECK(resultSize);

    // auto lfs = lfs_t{};
    // auto errorCode = lfs_format(&lfs, &sts1cobcsw::fs::lfsConfig);
    // REQUIRE(errorCode == 0);
    // errorCode = lfs_mount(&lfs, &sts1cobcsw::fs::lfsConfig);
    // REQUIRE(errorCode == 0);

    // auto const * directoryPath = "MyFolder";
    // errorCode = lfs_mkdir(&lfs, directoryPath);
    // REQUIRE(errorCode == 0);

    // // auto const * filePath = "MyFolder/MyFile";
    // auto file = lfs_file_t{};
    // errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_WRONLY | LFS_O_CREAT);
    // REQUIRE(errorCode == 0);

    // int number = 123;
    // errorCode = lfs_file_write(&lfs, &file, &number, sizeof(number));
    // REQUIRE(errorCode == sizeof(number));

    // errorCode = lfs_file_close(&lfs, &file);
    // REQUIRE(errorCode == 0);
    // errorCode = lfs_unmount(&lfs);
    // REQUIRE(errorCode == 0);

    // errorCode = lfs_mount(&lfs, &sts1cobcsw::fs::lfsConfig);
    // REQUIRE(errorCode == 0);
    // errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_RDONLY);
    // REQUIRE(errorCode == 0);

    // int readNumber = 0;
    // errorCode = lfs_file_read(&lfs, &file, &readNumber, sizeof(number));
    // REQUIRE(errorCode == sizeof(number));
    // REQUIRE(readNumber == number);

    // errorCode = lfs_file_close(&lfs, &file);
    // REQUIRE(errorCode == 0);
}
