#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#ifdef __linux__
    #include <Sts1CobcSw/FileSystem/LfsRam.hpp>
#endif
#include <Sts1CobcSw/FileSystem/LfsWrapper.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <littlefs/lfs.h>

#include <etl/to_string.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <vector>


namespace fs = sts1cobcsw::fs;

using sts1cobcsw::Span;
using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)


namespace
{
auto VerifyDataInMemory(std::span<const sts1cobcsw::Byte> dataToCheck) -> bool;
#ifdef __linux__
auto TryToCorruptDataInMemory(std::span<const sts1cobcsw::Byte> dataToCorrupt) -> bool;
#endif
}


TEST_CASE("LfsWrapper without data corruption")
{
    fs::Initialize();
    auto mountResult = fs::Mount();
    CHECK(not mountResult.has_error());

    auto dirPath = fs::Path("/MyDir");
    auto nonExistingPath = fs::Path("/Path/To/Wrong");

    auto createDirResult = fs::CreateDirectory(nonExistingPath);
    CHECK(createDirResult.has_error());

    // Dir does not exist
    auto iteratorResult = fs::MakeIterator(nonExistingPath);
    CHECK(iteratorResult.has_error());
    CHECK(iteratorResult.error() == sts1cobcsw::ErrorCode::noDirectoryEntry);

    createDirResult = fs::CreateDirectory(dirPath);
    CHECK(not createDirResult.has_error());

    auto filePath = fs::Path("/MyDir/MyFile");
    auto openResult = fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
    CHECK(openResult.has_value());
    auto & writeableFile = openResult.value();

    // Reopening the file should fail
    auto reopenResult = fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
    CHECK(reopenResult.has_error());

    // Empty file should have size 0
    auto sizeResult = writeableFile.Size();
    CHECK(sizeResult.has_value());
    CHECK(sizeResult.value() == 0);

    auto writeData = std::array{0xAA_b, 0xBB_b, 0xCC_b, 0xDD_b};
    auto writeResult = writeableFile.Write(Span(writeData));
    CHECK(writeResult.has_value());
    CHECK(writeResult.value() == static_cast<int>(writeData.size()));

    // Read() should fail since the file is only opened for writing
    auto readData = std::array{0x11_b, 0x22_b, 0x33_b, 0x44_b};
    auto readResult = writeableFile.Read(Span(&readData));
    CHECK(readResult.has_error());

    CHECK(not VerifyDataInMemory(writeData));
    auto flushResult = writeableFile.Flush();
    CHECK(not flushResult.has_error());
    CHECK(VerifyDataInMemory(writeData));

    // Remove should fail because the file is opened
    auto removeResult = fs::Remove(filePath);
    CHECK(removeResult.has_error());

    // Remove should fail because the file does not exist
    removeResult = fs::Remove(nonExistingPath);
    CHECK(removeResult.has_error());

    // ForceRemove should fail because the file does not exist
    removeResult = fs::ForceRemove(nonExistingPath);
    CHECK(removeResult.has_error());

    iteratorResult = fs::MakeIterator(dirPath);
    CHECK(not iteratorResult.has_error());
    auto & dirIterator = iteratorResult.value();
    CHECK(dirIterator != dirIterator.end());

    // Entry 0: 0 byte "."
    auto entryResult = *dirIterator;
    CHECK(not entryResult.has_error());
    auto entry = entryResult.value();
    CHECK(entry.size == static_cast<lfs_size_t>(0));
    ++dirIterator;

    // Entry 1: 0 byte ".."
    entryResult = *dirIterator;
    CHECK(not entryResult.has_error());
    entry = entryResult.value();
    CHECK(entry.size == static_cast<lfs_size_t>(0));
    ++dirIterator;

    // Entry 2: 0 byte "MyFile.lock"
    entryResult = *dirIterator;
    CHECK(not entryResult.has_error());
    entry = entryResult.value();
    CHECK(entry.size == static_cast<lfs_size_t>(0));
    ++dirIterator;

    // Entry 3: 4 Byte "MyFile"
    entryResult = *dirIterator;
    CHECK(not entryResult.has_error());
    entry = entryResult.value();
    CHECK(entry.size == static_cast<lfs_size_t>(4));

    auto dirIteratorCopy = dirIterator;
    ++dirIterator;

    // Should fail because we are at the end of the directory
    entryResult = *dirIterator;
    CHECK(entryResult.has_error());
    CHECK(dirIterator == dirIterator.end());

    // The copied iterator should still be at entry 3: 4 Byte "MyFile"
    CHECK(dirIteratorCopy != dirIterator.end());
    entryResult = *dirIteratorCopy;
    CHECK(not entryResult.has_error());
    entry = entryResult.value();
    CHECK(entry.size == static_cast<lfs_size_t>(4));

    auto secondDirIteratorCopy{dirIteratorCopy};
    CHECK(secondDirIteratorCopy == dirIteratorCopy);

    int entryCount = 0;
    iteratorResult = fs::MakeIterator(dirPath);
    CHECK(not iteratorResult.has_error());
    for(auto forEntryResult : iteratorResult.value())
    {
        CHECK(not forEntryResult.has_error());
        entryCount++;
    }
    // Range based for loop should go over all 4 entries
    CHECK(entryCount == 4);

    auto closeResult = writeableFile.Close();
    CHECK(not closeResult.has_error());

    // Flush() should fail because the file is closed
    flushResult = writeableFile.Flush();
    CHECK(flushResult.has_error());

    openResult = fs::Open(filePath, LFS_O_RDONLY);
    CHECK(openResult.has_value());
    auto & readableFile = openResult.value();

    sizeResult = readableFile.Size();
    CHECK(sizeResult.has_value());
    CHECK(sizeResult.value() == static_cast<int>(writeData.size()));

    readResult = readableFile.Read(Span(&readData));
    CHECK(readResult.has_value());
    CHECK(readResult.value() == static_cast<int>(readData.size()));
    CHECK(readData == writeData);

    // Write() should fail since the file is only opened for reading
    writeResult = readableFile.Write(Span(writeData));
    CHECK(writeResult.has_error());

    // Flush() should fail since the file is only opened for reading
    flushResult = readableFile.Flush();
    CHECK(flushResult.has_error());

    closeResult = readableFile.Close();
    CHECK(not closeResult.has_error());

    removeResult = fs::Remove(filePath);
    CHECK(not removeResult.has_error());

    openResult = fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
    CHECK(openResult.has_value());
    auto & deletedFile = openResult.value();

    sizeResult = deletedFile.Size();
    CHECK(sizeResult.has_value());
    CHECK(sizeResult.value() == 0);

    // Write file and remove it with ForceRemove() while still open
    writeResult = deletedFile.Write(Span(writeData));
    CHECK(not writeResult.has_error());

    removeResult = fs::ForceRemove(filePath);
    CHECK(not removeResult.has_error());

    closeResult = deletedFile.Close();
    CHECK(closeResult.has_error());
    CHECK(closeResult.error() == sts1cobcsw::ErrorCode::noDirectoryEntry);

    removeResult = fs::Remove(dirPath);
    CHECK(not removeResult.has_error());

    auto unmountResult = fs::Unmount();
    CHECK(not unmountResult.has_error());
}


#ifdef __linux__
TEST_CASE("LfsWrapper with data corruption")
{
    fs::Initialize();

    // Test with faulty write
    {
        auto mountResult = fs::Mount();
        CHECK(not mountResult.has_error());

        auto filePath = fs::Path("/FaultyWrite");
        auto openResult = fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
        CHECK(openResult.has_value());
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
        CHECK(writeResult.has_value());
        CHECK(writeResult.value() == static_cast<int>(writeData.size()));

        auto closeResult = file.Close();
        CHECK(not closeResult.has_error());
        // Reset the program finished handler after closing the file because that is when the data
        // is really written to memory
        fs::SetProgramFinishedHandler(nullptr);
        openResult = fs::Open(filePath, LFS_O_RDONLY);
        CHECK(openResult.has_value());
        auto & corruptedFile = openResult.value();

        // Since littlefs detected and corrected the write fault, we should read the correct data
        auto readData = std::array{0x11_b, 0x22_b, 0x33_b, 0x44_b};
        auto readResult = corruptedFile.Read(Span(&readData));
        CHECK(readResult.has_value());
        CHECK(readResult.value() == static_cast<int>(readData.size()));
        CHECK(readData == writeData);

        closeResult = corruptedFile.Close();
        CHECK(not closeResult.has_error());
        auto unmountResult = fs::Unmount();
        CHECK(not unmountResult.has_error());
    }

    // Test with bit flip (once while mounted and once while unmounted)
    for(auto i = 0; i < 2; ++i)
    {
        auto mountResult = fs::Mount();
        CHECK(not mountResult.has_error());

        auto filePath = fs::Path("/BitFlip");
        etl::to_string(i, filePath, /*append=*/true);
        auto openResult = fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
        CHECK(openResult.has_value());
        auto & file = openResult.value();

        static constexpr auto writeData = std::array{0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b};
        auto writeResult = file.Write(Span(writeData));
        CHECK(writeResult.has_value());
        CHECK(writeResult.value() == static_cast<int>(writeData.size()));

        auto closeResult = file.Close();
        CHECK(not closeResult.has_error());
        if(i == 0)
        {
            auto unmountResult = fs::Unmount();
            CHECK(not unmountResult.has_error());
        }
        auto dataWasFoundAndCorrupted = TryToCorruptDataInMemory(writeData);
        CHECK(dataWasFoundAndCorrupted);
        if(i == 0)
        {
            auto corruptedMountResult = fs::Mount();
            CHECK(not corruptedMountResult.has_error());
        }
        openResult = fs::Open(filePath, LFS_O_RDONLY);
        CHECK(not openResult.has_error());
        auto & corruptedFile = openResult.value();

        // File is empty, but it can be read (i.e. Read() does not fail)
        auto sizeResult = corruptedFile.Size();
        CHECK(sizeResult.has_value());
        CHECK(sizeResult.value() == 0);
        auto readData = decltype(writeData){};
        auto readResult = corruptedFile.Read(Span(&readData));
        CHECK(readResult.has_value());
        CHECK(readResult.value() == 0);
        CHECK(readData != writeData);

        closeResult = corruptedFile.Close();
        CHECK(not closeResult.has_error());
        auto unmountResult = fs::Unmount();
        CHECK(not unmountResult.has_error());
    }
}
#endif


namespace
{
auto VerifyDataInMemory([[maybe_unused]] std::span<const sts1cobcsw::Byte> dataToCheck) -> bool
{
#ifdef __linux__
    auto it =
        std::search(fs::memory.begin(), fs::memory.end(), dataToCheck.begin(), dataToCheck.end());
    return static_cast<bool>(it != fs::memory.end());
#else
    return true;
#endif
}


#ifdef __linux__
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
#endif
}
