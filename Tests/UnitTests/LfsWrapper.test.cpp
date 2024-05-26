#include <catch2/catch_test_macros.hpp>
#include <Sts1CobcSw/FileSystem/LfsStorageDevice.hpp>
//#include <Sts1CobcSw/FileSystem/LfsWrapper.hpp>
//#include <Sts1CobcSw/FileSystem/ErrorsAndResult.hpp>


#include <littlefs/lfs.h>


//TEST_CASE("LfsWrapper")
//{
//    sts1cobcsw::fs::Initialize();
// 
    // auto lfs = lfs_t{};
    // auto errorCode = lfs_format(&lfs, &sts1cobcsw::fs::lfsConfig);
    // CHECK(errorCode == 0);
// 
    // auto result = sts1cobcsw::fs::Mount();
    // CHECK(result);
// 
    // auto const * filePath = "MyFile";
    // auto resultFileOpen = sts1cobcsw::fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
    // CHECK(resultFileOpen);
    // sts1cobcsw::fs::File & file = resultFileOpen.value();
    // result = file.Close();
    // CHECK(result);

    /*
    int number = 123;
    auto resultWrite = file.Write(number);
    CHECK(resultWrite);

    auto resultSize = file.Size();
    CHECK(resultSize); */

    //
    // auto lfs = lfs_t{};
    // auto errorCode = lfs_format(&lfs, &sts1cobcsw::fs::lfsConfig);
    // REQUIRE(errorCode == 0);
    // errorCode = lfs_mount(&lfs, &sts1cobcsw::fs::lfsConfig);
    // REQUIRE(errorCode == 0);
    //
    // auto const * directoryPath = "MyFolder";
    // errorCode = lfs_mkdir(&lfs, directoryPath);
    // REQUIRE(errorCode == 0);
    //
    //// auto const * filePath = "MyFolder/MyFile";
    // auto file = lfs_file_t{};
    // errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_WRONLY | LFS_O_CREAT);
    // REQUIRE(errorCode == 0);
    //
    // int number = 123;
    // errorCode = lfs_file_write(&lfs, &file, &number, sizeof(number));
    // REQUIRE(errorCode == sizeof(number));
    //
    // errorCode = lfs_file_close(&lfs, &file);
    // REQUIRE(errorCode == 0);
    // errorCode = lfs_unmount(&lfs);
    // REQUIRE(errorCode == 0);
    //
    // errorCode = lfs_mount(&lfs, &sts1cobcsw::fs::lfsConfig);
    // REQUIRE(errorCode == 0);
    // errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_RDONLY);
    // REQUIRE(errorCode == 0);
    //
    // int readNumber = 0;
    // errorCode = lfs_file_read(&lfs, &file, &readNumber, sizeof(number));
    // REQUIRE(errorCode == sizeof(number));
    // REQUIRE(readNumber == number);
    //
    // errorCode = lfs_file_close(&lfs, &file);
    // REQUIRE(errorCode == 0);
//}
