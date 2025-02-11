#include <Tests/UnitTests/UnitTestThread.hpp>

#include <Sts1CobcSw/FileSystem/ErrorsAndResult.hpp>
#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#include <Sts1CobcSw/FileSystem/LfsRam.hpp>
#include <Sts1CobcSw/FileSystem/LfsWrapper.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <littlefs/lfs.h>

#include <etl/string.h>
#include <etl/to_string.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>


namespace fs = sts1cobcsw::fs;

using sts1cobcsw::Span;
using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)


auto TryToCorruptDataInMemory(std::span<const sts1cobcsw::Byte> dataToCorrupt) -> bool;


auto RunUnitTest() -> void
{
    fs::Initialize();

    // Test without memory corruption or faults
    {
        auto mountResult = fs::Mount();
        Require(not mountResult.has_error());

        auto filePath = fs::Path("/MyFile");
        auto openResult = fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
        Require(openResult.has_value());
        auto & writeableFile = openResult.value();

        // Reopening the file should fail
        auto reopenResult = fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
        Require(reopenResult.has_error());

        // Empty file should have size 0
        auto sizeResult = writeableFile.Size();
        Require(sizeResult.has_value());
        Require(sizeResult.value() == 0);

        auto writeData = std::array{0xAA_b, 0xBB_b, 0xCC_b, 0xDD_b};
        auto writeResult = writeableFile.Write(Span(writeData));
        Require(writeResult.has_value());
        Require(writeResult.value() == static_cast<int>(writeData.size()));

        // Read() should fail since the file is only opened for writing
        auto readData = std::array{0x11_b, 0x22_b, 0x33_b, 0x44_b};
        auto readResult = writeableFile.Read(Span(&readData));
        Require(readResult.has_error());

        auto closeResult = writeableFile.Close();
        Require(not closeResult.has_error());

        openResult = fs::Open(filePath, LFS_O_RDONLY);
        Require(openResult.has_value());
        auto & readableFile = openResult.value();

        sizeResult = readableFile.Size();
        Require(sizeResult.has_value());
        Require(sizeResult.value() == static_cast<int>(writeData.size()));

        readResult = readableFile.Read(Span(&readData));
        Require(readResult.has_value());
        Require(readResult.value() == static_cast<int>(readData.size()));
        Require(readData == writeData);

        // Write() should fail since the file is only opened for reading
        writeResult = readableFile.Write(Span(writeData));
        Require(writeResult.has_error());

        closeResult = readableFile.Close();
        Require(not closeResult.has_error());
        auto unmountResult = fs::Unmount();
        Require(not unmountResult.has_error());
    }

    // Test with faulty write
    {
        auto mountResult = fs::Mount();
        Require(not mountResult.has_error());

        auto filePath = fs::Path("/FaultyWrite");
        auto openResult = fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
        Require(openResult.has_value());
        auto & file = openResult.value();

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
        auto writeResult = file.Write(Span(writeData));
        Require(writeResult.has_value());
        Require(writeResult.value() == static_cast<int>(writeData.size()));

        auto closeResult = file.Close();
        Require(not closeResult.has_error());
        // Reset the program finished handler after closing the file because that is when the data
        // is really written to memory
        fs::SetProgramFinishedHandler(nullptr);
        openResult = fs::Open(filePath, LFS_O_RDONLY);
        Require(openResult.has_value());
        auto & corruptedFile = openResult.value();

        // Since littlefs detected and corrected the write fault, we should read the correct data
        auto readData = std::array{0x11_b, 0x22_b, 0x33_b, 0x44_b};
        auto readResult = corruptedFile.Read(Span(&readData));
        Require(readResult.has_value());
        Require(readResult.value() == static_cast<int>(readData.size()));
        Require(readData == writeData);

        closeResult = corruptedFile.Close();
        Require(not closeResult.has_error());
        auto unmountResult = fs::Unmount();
        Require(not unmountResult.has_error());
    }

    // Test with bit flip (once while mounted and once while unmounted)
    for(auto i = 0; i < 2; ++i)
    {
        auto mountResult = fs::Mount();
        Require(not mountResult.has_error());

        auto filePath = fs::Path("/BitFlip");
        etl::to_string(i, filePath, /*append=*/true);
        auto openResult = fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
        Require(openResult.has_value());
        auto & file = openResult.value();

        static constexpr auto writeData = std::array{0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b};
        auto writeResult = file.Write(Span(writeData));
        Require(writeResult.has_value());
        Require(writeResult.value() == static_cast<int>(writeData.size()));

        auto closeResult = file.Close();
        Require(not closeResult.has_error());
        if(i == 0)
        {
            auto unmountResult = fs::Unmount();
            Require(not unmountResult.has_error());
        }
        auto dataWasFoundAndCorrupted = TryToCorruptDataInMemory(writeData);
        Require(dataWasFoundAndCorrupted);
        if(i == 0)
        {
            auto corruptedMountResult = fs::Mount();
            Require(not corruptedMountResult.has_error());
        }
        openResult = fs::Open(filePath, LFS_O_RDONLY);
        Require(not openResult.has_error());
        auto & corruptedFile = openResult.value();

        // File is empty, but it can be read (i.e. Read() does not fail)
        auto sizeResult = corruptedFile.Size();
        Require(sizeResult.has_value());
        Require(sizeResult.value() == 0);
        auto readData = decltype(writeData){};
        auto readResult = corruptedFile.Read(Span(&readData));
        Require(readResult.has_value());
        Require(readResult.value() == 0);
        Require(readData != writeData);

        closeResult = corruptedFile.Close();
        Require(not closeResult.has_error());
        auto unmountResult = fs::Unmount();
        Require(not unmountResult.has_error());
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
