#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/FileSystem/DirectoryIterator.hpp>
#include <Sts1CobcSw/FileSystem/File.hpp>
#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#ifdef __linux__
    #include <Sts1CobcSw/FileSystem/LfsRam.hpp>
#endif
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <littlefs/lfs.h>

#include <etl/string.h>
#include <etl/to_string.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <span>
#include <vector>


namespace fs = sts1cobcsw::fs;

using sts1cobcsw::ErrorCode;
using sts1cobcsw::Span;
using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)


namespace
{
auto CheckIfDataIsInMemory(std::span<const sts1cobcsw::Byte> dataToCheck) -> void;
auto CheckIfDataIsNotInMemory(std::span<const sts1cobcsw::Byte> dataToCheck) -> void;
#ifdef __linux__
auto TryToCorruptDataInMemory(std::span<const sts1cobcsw::Byte> dataToCorrupt) -> bool;
#endif
}


TEST_CASE("FileSystem without data corruption")
{
    fs::Initialize();
    auto mountResult = fs::Mount();
    REQUIRE(mountResult.has_error() == false);

    auto nonExistingPath = fs::Path("/Path/To/Wrong");
    auto createDirResult = fs::CreateDirectory(nonExistingPath);
    CHECK(createDirResult.has_error());
    CHECK(createDirResult.error() == ErrorCode::notFound);

    auto makeIteratorResult = fs::MakeIterator(nonExistingPath);
    CHECK(makeIteratorResult.has_error());
    CHECK(makeIteratorResult.error() == ErrorCode::notFound);

    auto dirPath = fs::Path("/MyDir");
    createDirResult = fs::CreateDirectory(dirPath);
    CHECK(createDirResult.has_error() == false);

    auto filePath = fs::Path("/MyDir/MyFile");
    auto openResult = fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
    CHECK(openResult.has_value());
    auto & writeableFile = openResult.value();

    auto reopenResult = fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
    CHECK(reopenResult.has_error());
    CHECK(reopenResult.error() == ErrorCode::fileLocked);

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
    CHECK(readResult.error() == ErrorCode::unsupportedOperation);

    CheckIfDataIsNotInMemory(writeData);
    auto flushResult = writeableFile.Flush();
    CHECK(flushResult.has_error() == false);
    CheckIfDataIsInMemory(writeData);

    auto seekResult = writeableFile.SeekAbsolute(-2);
    CHECK(seekResult.has_error());
    CHECK(seekResult.error() == ErrorCode::invalidParameter);

    seekResult = writeableFile.SeekRelative(-3);
    CHECK(seekResult.has_error() == false);
    CHECK(seekResult.value() == 1);

    seekResult = writeableFile.SeekAbsolute(3);
    CHECK(seekResult.has_error() == false);
    CHECK(seekResult.value() == 3);

    writeResult = writeableFile.Write(Span(0x12_b));
    CHECK(writeResult.has_value());
    CHECK(writeResult.value() == 1);

    auto removeResult = fs::Remove(filePath);
    CHECK(removeResult.has_error());
    CHECK(removeResult.error() == ErrorCode::fileLocked);

    removeResult = fs::Remove(nonExistingPath);
    CHECK(removeResult.has_error());
    CHECK(removeResult.error() == ErrorCode::notFound);

    removeResult = fs::ForceRemove(nonExistingPath);
    CHECK(removeResult.has_error());
    CHECK(removeResult.error() == ErrorCode::notFound);

    makeIteratorResult = fs::MakeIterator(dirPath);
    CHECK(makeIteratorResult.has_error() == false);
    auto & dirIterator = makeIteratorResult.value();
    CHECK(dirIterator != dirIterator.end());

    // Entry 0: 0 B, "."
    auto entryResult = *dirIterator;
    CHECK(entryResult.has_error() == false);
    auto entry = entryResult.value();
    CHECK(entry.type == fs::EntryType::directory);
    CHECK(entry.name == ".");
    CHECK(entry.size == 0U);
    ++dirIterator;

    // Entry 1: 0 B, ".."
    entryResult = *dirIterator;
    CHECK(entryResult.has_error() == false);
    entry = entryResult.value();
    CHECK(entry.type == fs::EntryType::directory);
    CHECK(entry.name == "..");
    CHECK(entry.size == 0U);
    ++dirIterator;

    // Entry 2: 0 B, "MyFile.lock"
    entryResult = *dirIterator;
    CHECK(entryResult.has_error() == false);
    entry = entryResult.value();
    CHECK(entry.type == fs::EntryType::file);
    CHECK(entry.name == "MyFile.lock");
    CHECK(entry.size == 0U);
    ++dirIterator;

    // Entry 3: 4 B, "MyFile"
    entryResult = *dirIterator;
    CHECK(entryResult.has_error() == false);
    entry = entryResult.value();
    CHECK(entry.type == fs::EntryType::file);
    CHECK(entry.name == "MyFile");
    CHECK(entry.size == 4U);

    auto dirIteratorCopy = dirIterator;
    ++dirIterator;

    // Should fail because we are at the end of the directory
    entryResult = *dirIterator;
    CHECK(entryResult.has_error());
    CHECK(entryResult.error() == ErrorCode::unsupportedOperation);
    CHECK(dirIterator == dirIterator.end());

    // The copied iterator should still be at entry 3: 4 B, "MyFile"
    CHECK(dirIteratorCopy != dirIterator.end());
    entryResult = *dirIteratorCopy;
    CHECK(entryResult.has_error() == false);
    entry = entryResult.value();
    CHECK(entry.type == fs::EntryType::file);
    CHECK(entry.name == "MyFile");
    CHECK(entry.size == 4U);

    auto dirIteratorCopy2 = dirIteratorCopy;
    CHECK(dirIteratorCopy2 == dirIteratorCopy);

    auto nEntries = 0;
    makeIteratorResult = fs::MakeIterator(dirPath);
    CHECK(makeIteratorResult.has_error() == false);
    for(auto const & forEntryResult : makeIteratorResult.value())
    {
        CHECK(forEntryResult.has_error() == false);
        nEntries++;
    }
    CHECK(nEntries == 4);

    makeIteratorResult = fs::MakeIterator(dirPath);
    CHECK(makeIteratorResult.has_error() == false);
    dirIterator = makeIteratorResult.value();
    nEntries = std::distance(dirIterator, dirIterator.end());
    CHECK(nEntries == 4);

    auto closeResult = writeableFile.Close();
    CHECK(closeResult.has_error() == false);

    sizeResult = writeableFile.Size();
    CHECK(sizeResult.has_error());
    CHECK(sizeResult.error() == ErrorCode::fileNotOpen);

    flushResult = writeableFile.Flush();
    CHECK(flushResult.has_error());
    CHECK(flushResult.error() == ErrorCode::fileNotOpen);

    seekResult = writeableFile.SeekAbsolute(0);
    CHECK(seekResult.has_error());
    CHECK(seekResult.error() == ErrorCode::fileNotOpen);

    openResult = fs::Open(filePath, LFS_O_RDONLY);
    CHECK(openResult.has_value());
    auto & readableFile = openResult.value();

    sizeResult = readableFile.Size();
    CHECK(sizeResult.has_value());
    CHECK(sizeResult.value() == 4);

    readResult = readableFile.Read(Span(&readData));
    CHECK(readResult.has_value());
    CHECK(readResult.value() == static_cast<int>(readData.size()));
    CHECK(readData == (std::array{0xAA_b, 0xBB_b, 0xCC_b, 0x12_b}));

    seekResult = readableFile.SeekRelative(-1);
    CHECK(seekResult.has_error() == false);
    CHECK(seekResult.value() == 3);

    seekResult = readableFile.SeekAbsolute(2);
    CHECK(seekResult.has_error() == false);
    CHECK(seekResult.value() == 2);

    readData = {};
    readResult = readableFile.Read(Span(&readData));
    CHECK(readResult.has_value());
    CHECK(readResult.value() == 2);
    CHECK(readData == (std::array{0xCC_b, 0x12_b, 0x00_b, 0x00_b}));

    // Write() should fail since the file is only opened for reading
    writeResult = readableFile.Write(Span(writeData));
    CHECK(writeResult.has_error());
    CHECK(writeResult.error() == ErrorCode::unsupportedOperation);

    // Flush() should fail since the file is only opened for reading
    flushResult = readableFile.Flush();
    CHECK(flushResult.has_error());
    CHECK(flushResult.error() == ErrorCode::unsupportedOperation);

    closeResult = readableFile.Close();
    CHECK(closeResult.has_error() == false);

    removeResult = fs::Remove(filePath);
    CHECK(removeResult.has_error() == false);

    openResult = fs::Open(filePath, LFS_O_WRONLY | LFS_O_CREAT);
    CHECK(openResult.has_value());
    auto & deletedFile = openResult.value();

    sizeResult = deletedFile.Size();
    CHECK(sizeResult.has_value());
    CHECK(sizeResult.value() == 0);

    writeResult = deletedFile.Write(Span(writeData));
    CHECK(writeResult.has_error() == false);

    // ForceRemove() works even if the file is open
    removeResult = fs::ForceRemove(filePath);
    CHECK(removeResult.has_error() == false);

    closeResult = deletedFile.Close();
    CHECK(closeResult.has_error());
    CHECK(closeResult.error() == ErrorCode::notFound);

    removeResult = fs::Remove(dirPath);
    CHECK(removeResult.has_error() == false);

    auto unmountResult = fs::Unmount();
    CHECK(unmountResult.has_error() == false);
}


#ifdef __linux__
TEST_CASE("FileSystem with data corruption")
{
    fs::Initialize();

    // Test with faulty write
    {
        auto mountResult = fs::Mount();
        CHECK(mountResult.has_error() == false);

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
        CHECK(closeResult.has_error() == false);
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
        CHECK(closeResult.has_error() == false);
        auto unmountResult = fs::Unmount();
        CHECK(unmountResult.has_error() == false);
    }

    // Test with bit flip (once while mounted and once while unmounted)
    for(auto i = 0; i < 2; ++i)
    {
        auto mountResult = fs::Mount();
        CHECK(mountResult.has_error() == false);

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
        CHECK(closeResult.has_error() == false);
        if(i == 0)
        {
            auto unmountResult = fs::Unmount();
            CHECK(unmountResult.has_error() == false);
        }
        auto dataWasFoundAndCorrupted = TryToCorruptDataInMemory(writeData);
        CHECK(dataWasFoundAndCorrupted);
        if(i == 0)
        {
            auto corruptedMountResult = fs::Mount();
            CHECK(corruptedMountResult.has_error() == false);
        }
        openResult = fs::Open(filePath, LFS_O_RDONLY);
        CHECK(openResult.has_error() == false);
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
        CHECK(closeResult.has_error() == false);
        auto unmountResult = fs::Unmount();
        CHECK(unmountResult.has_error() == false);
    }
}
#endif


namespace
{
auto CheckIfDataIsInMemory([[maybe_unused]] std::span<const sts1cobcsw::Byte> dataToCheck) -> void
{
#ifdef __linux__
    auto it =
        std::search(fs::memory.begin(), fs::memory.end(), dataToCheck.begin(), dataToCheck.end());
    CHECK(it != fs::memory.end());
#endif
}


auto CheckIfDataIsNotInMemory([[maybe_unused]] std::span<const sts1cobcsw::Byte> dataToCheck)
    -> void
{
#ifdef __linux__
    auto it =
        std::search(fs::memory.begin(), fs::memory.end(), dataToCheck.begin(), dataToCheck.end());
    CHECK(it == fs::memory.end());
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
