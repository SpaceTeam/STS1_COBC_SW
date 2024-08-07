#include <Sts1CobcSw/FileSystem/ErrorsAndResult.hpp>
#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#include <Sts1CobcSw/FileSystem/LfsWrapper.hpp>

#include <catch2/catch_test_macros.hpp>

#include <littlefs/lfs.h>

#include <Sts1CobcSw/FileSystem/LfsWrapper.ipp>


TEST_CASE("LfsWrapper")
{
    CHECK(true);
    sts1cobcsw::fs::Initialize();
    auto mountResult = sts1cobcsw::fs::Mount();
    CHECK(not mountResult.has_error());

    auto const * filePath = "/MyFile";
    auto openResult = sts1cobcsw::fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
    CHECK(openResult.has_value());

    auto & writeableFile = openResult.value();

    int const number = 123;
    auto writeResult = writeableFile.Write(number);
    CHECK(writeResult.has_value());
    CHECK(writeResult.value() == sizeof(number));

    int readNumber = 0;
    auto readResult = writeableFile.Read(&readNumber);
    CHECK(readResult.has_error());  // read file should fail as LFS_O_WRONLY flag used

    auto closeResult = writeableFile.Close();
    CHECK(not closeResult.has_error());

    openResult = sts1cobcsw::fs::Open(filePath, LFS_O_RDONLY | LFS_O_CREAT);
    CHECK(openResult.has_value());

    auto & readableFile = openResult.value();

    auto sizeResult = readableFile.Size();
    CHECK(sizeResult.has_value());
    CHECK(sizeResult.value() == sizeof(int));

    readResult = readableFile.Read(&readNumber);
    CHECK(readResult.has_value());
    CHECK(readResult.value() == sizeof(number));
    CHECK(readNumber == number);

    writeResult = readableFile.Write(number);
    CHECK(writeResult.has_error());  // write file should fail as LFS_O_RDONLY flag used

    closeResult = readableFile.Close();
    CHECK(not closeResult.has_error());

    auto unmountResult = sts1cobcsw::fs::Unmount();
    CHECK(not unmountResult.has_error());
}
