#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>

#include <littlefs/lfs.h>

#include <etl/string.h>


namespace fs = sts1cobcsw::fs;

using Path = etl::string<fs::maxPathLength>;


TEST_CASE("Littlefs")
{
    fs::Initialize();
    auto lfs = lfs_t{};
    auto errorCode = lfs_format(&lfs, &fs::lfsConfig);
    CHECK(errorCode == 0);
    errorCode = lfs_mount(&lfs, &fs::lfsConfig);
    REQUIRE(errorCode == 0);

    auto const * directoryPath = "MyFolder";
    errorCode = lfs_mkdir(&lfs, directoryPath);
    CHECK(errorCode == 0);
    auto filePath = Path(directoryPath);
    filePath.append("/MyFile");
    auto file = lfs_file_t{};
    errorCode = lfs_file_open(&lfs, &file, filePath.c_str(), LFS_O_WRONLY | LFS_O_CREAT);
    CHECK(errorCode == 0);
    static constexpr auto writtenNumber = 123;
    errorCode = lfs_file_write(&lfs, &file, &writtenNumber, sizeof(writtenNumber));
    CHECK(errorCode == static_cast<int>(sizeof(writtenNumber)));
    errorCode = lfs_file_close(&lfs, &file);
    CHECK(errorCode == 0);

    errorCode = lfs_unmount(&lfs);
    CHECK(errorCode == 0);
    errorCode = lfs_mount(&lfs, &fs::lfsConfig);
    CHECK(errorCode == 0);

    file = lfs_file_t{};
    errorCode = lfs_file_open(&lfs, &file, filePath.c_str(), LFS_O_RDONLY);
    CHECK(errorCode == 0);
    auto readNumber = 0;
    errorCode = lfs_file_read(&lfs, &file, &readNumber, sizeof(readNumber));
    CHECK(errorCode == static_cast<int>(sizeof(readNumber)));
    CHECK(readNumber == writtenNumber);
    errorCode = lfs_file_close(&lfs, &file);
    CHECK(errorCode == 0);

    errorCode = lfs_remove(&lfs, directoryPath);
    CHECK(errorCode == LFS_ERR_NOTEMPTY);
    errorCode = lfs_remove(&lfs, "NonExistentFolder/");
    CHECK(errorCode == LFS_ERR_NOENT);
    errorCode = lfs_remove(&lfs, "NonExistentFile.txt");
    CHECK(errorCode == LFS_ERR_NOENT);
    errorCode = lfs_remove(&lfs, filePath.c_str());
    CHECK(errorCode == 0);
    errorCode = lfs_remove(&lfs, directoryPath);
    CHECK(errorCode == 0);

    errorCode = lfs_unmount(&lfs);
    CHECK(errorCode == 0);
}
