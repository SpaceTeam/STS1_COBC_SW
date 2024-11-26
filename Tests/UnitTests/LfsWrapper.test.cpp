#include <Tests/UnitTests/UnitTestThread.hpp>

#include <Sts1CobcSw/FileSystem/ErrorsAndResult.hpp>
#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#include <Sts1CobcSw/FileSystem/LfsWrapper.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <littlefs/lfs.h>

#include <etl/string.h>

#include <algorithm>
#include <array>


using sts1cobcsw::Byte;
using sts1cobcsw::Span;
using sts1cobcsw::fs::Path;
using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)


auto RunUnitTest() -> void
{
    Require(true);

    sts1cobcsw::fs::Initialize();
    auto mountResult = sts1cobcsw::fs::Mount();
    Require(not mountResult.has_error());

    auto filePath = Path("/MyFile");
    auto openResult = sts1cobcsw::fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
    Require(openResult.has_value());
    auto & writeableFile = openResult.value();

    // Reopening the file should fail
    auto reopenResult = sts1cobcsw::fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
    Require(reopenResult.has_error());

    // Empty file should have size 0
    auto sizeResult = writeableFile.Size();
    Require(sizeResult.has_value());
    Require(sizeResult.value() == 0);

    auto writeData = std::array{0xAA_b, 0xBB_b, 0xCC_b, 0xDD_b};
    auto writeResult = writeableFile.Write(Span(writeData));
    Require(writeResult.has_value());
    Require(writeResult.value() == sizeof(writeData));

    // Read() should fail since the file is only opened for writing
    auto readData = std::array{0x11_b, 0x22_b, 0x33_b, 0x44_b};
    auto readResult = writeableFile.Read(Span(&readData));
    Require(readResult.has_error());

    auto closeResult = writeableFile.Close();
    Require(not closeResult.has_error());

    openResult = sts1cobcsw::fs::Open(filePath, LFS_O_RDONLY);
    Require(openResult.has_value());
    auto & readableFile = openResult.value();

    sizeResult = readableFile.Size();
    Require(sizeResult.has_value());
    Require(sizeResult.value() == sizeof(writeData));

    readResult = readableFile.Read(Span(&readData));
    Require(readResult.has_value());
    Require(readResult.value() == sizeof(readData));
    Require(readData == writeData);

    // Write() should fail since the file is only opened for reading
    writeResult = readableFile.Write(Span(writeData));
    Require(writeResult.has_error());

    closeResult = readableFile.Close();
    Require(not closeResult.has_error());

    auto unmountResult = sts1cobcsw::fs::Unmount();
    Require(not unmountResult.has_error());
}
