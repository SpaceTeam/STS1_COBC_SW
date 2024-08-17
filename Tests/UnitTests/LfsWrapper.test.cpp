#include <Sts1CobcSw/FileSystem/ErrorsAndResult.hpp>
#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#include <Sts1CobcSw/FileSystem/LfsWrapper.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <catch2/catch_test_macros.hpp>

#include <littlefs/lfs.h>

#include <algorithm>
#include <array>

using sts1cobcsw::Byte;
using sts1cobcsw::Span;
using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)


TEST_CASE("LfsWrapper")
{
    sts1cobcsw::fs::Initialize();
    auto mountResult = sts1cobcsw::fs::Mount();
    CHECK(not mountResult.has_error());

    auto const * filePath = "/MyFile";
    auto openResult = sts1cobcsw::fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
    CHECK(openResult.has_value());

    auto & writeableFile = openResult.value();

    auto writeData = std::array{0xAA_b, 0xBB_b, 0xCC_b, 0xDD_b};
    auto writeResult = writeableFile.Write(Span(writeData));
    CHECK(writeResult.has_value());
    CHECK(writeResult.value() == sizeof(writeData));

    auto readData = std::array{0x11_b, 0x22_b, 0x33_b, 0x44_b};
    auto readResult = writeableFile.Read(Span(&readData));
    CHECK(readResult.has_error());  // read file should fail as LFS_O_WRONLY flag used

    auto closeResult = writeableFile.Close();
    CHECK(not closeResult.has_error());

    openResult = sts1cobcsw::fs::Open(filePath, LFS_O_RDONLY | LFS_O_CREAT);
    CHECK(openResult.has_value());

    auto & readableFile = openResult.value();

    auto sizeResult = readableFile.Size();
    CHECK(sizeResult.has_value());
    CHECK(sizeResult.value() == sizeof(int));

    readResult = readableFile.Read(Span(&readData));
    CHECK(readResult.has_value());
    CHECK(readResult.value() == sizeof(writeData));
    CHECK(readData == writeData);

    writeResult = readableFile.Write(Span(writeData));
    CHECK(writeResult.has_error());  // write file should fail as LFS_O_RDONLY flag used

    closeResult = readableFile.Close();
    CHECK(not closeResult.has_error());

    auto unmountResult = sts1cobcsw::fs::Unmount();
    CHECK(not unmountResult.has_error());
}
