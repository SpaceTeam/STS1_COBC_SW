#pragma once


#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>

#include <littlefs/lfs.h>

#include <etl/string.h>

#include <cstdint>
#include <iterator>


namespace sts1cobcsw::fs
{
enum class EntryType : std::uint8_t
{
    file = LFS_TYPE_REG,
    directory = LFS_TYPE_DIR,
};


struct DirectoryInfo
{
    EntryType type = EntryType::file;
    Path name = "";
    std::uint32_t size = 0U;

    friend auto operator==(DirectoryInfo const &, DirectoryInfo const &) -> bool = default;
};


class DirectoryIterator
{
public:
    // NOLINTBEGIN(readability-identifier-naming)
    using value_type = Result<DirectoryInfo>;
    using difference_type = int;
    using pointer = Result<DirectoryInfo> *;
    using reference = Result<DirectoryInfo> &;
    using iterator_category = std::input_iterator_tag;
    // NOLINTEND(readability-identifier-naming)

    DirectoryIterator(DirectoryIterator const & other) noexcept;
    DirectoryIterator(DirectoryIterator && other) noexcept;
    auto operator=(DirectoryIterator const & other) noexcept -> DirectoryIterator &;
    auto operator=(DirectoryIterator && other) noexcept -> DirectoryIterator &;
    ~DirectoryIterator();

    friend auto MakeIterator(Path const & path) -> Result<DirectoryIterator>;
    friend auto operator==(DirectoryIterator const & lhs, DirectoryIterator const & rhs) -> bool;

    // Post-increment returns by value → copy → too expensive for us so we don't implement it
    auto operator++() -> DirectoryIterator &;
    [[nodiscard]] auto operator*() const -> Result<DirectoryInfo>;

    [[nodiscard]] auto begin() const -> DirectoryIterator;  // NOLINT(readability-identifier-naming)
    [[nodiscard]] static auto end() -> DirectoryIterator;   // NOLINT(readability-identifier-naming)


private:
    DirectoryIterator() = default;
    auto CopyConstructFrom(DirectoryIterator const * other) noexcept -> void;
    auto ReadNextDirectoryEntry() -> void;
    // If an error occurs, we want to go into the default state like the end iterator but keep the
    // error code
    auto ResetEverythingExceptErrorCode() -> void;

    Path path_ = "";
    lfs_dir_t lfsDirectory_ = {};
    DirectoryInfo directoryInfo_ = {};
    int lfsErrorCode_ = 0;
};


[[nodiscard]] auto MakeIterator(Path const & path) -> Result<DirectoryIterator>;
}
