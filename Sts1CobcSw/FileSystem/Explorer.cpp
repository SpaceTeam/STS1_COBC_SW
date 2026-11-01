//Explorer.cpp

#include <Sts1CobcSw/FileSystem/Explorer.hpp>
#include <Sts1CobcSw/Flash/Flash.hpp>
#include <Sts1CobcSw/FileSystem/DirectoryIterator.hpp>
#include <Sts1CobcSw/FileSystem/File.hpp>
#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <cstdio>

using namespace sts1cobcsw::fs;

namespace sts1cobcsw::exp
{
    Path currentWorkingDirectory = "/";
}

// Check if initialization was successful

auto PrintWorkingDirectory() -> void // pwd
{
    printf("%s\n", currentWorkingDirectory.c_str());
}

auto ListDirectory(Path const & path) -> void // ls <dir>
{
    auto iteratorResult = MakeIterator(path);
    if(not iteratorResult)
    {
        printf("Error: could not open directory\n");
        return;
    }
    for(auto const & entryResult : iteratorResult.value())
    {
        if(not entryResult)
        {
            printf("Error reading entry\n");
            break;
        }
        auto const & entry = entryResult.value();

        if(entry.type == EntryType::file)
        {
            printf("[FILE] %s (%u bytes)\n", entry.name.c_str(), entry.size);
        }
        else
        {
            printf("[DIR]  %s\n", entry.name.c_str());
        }
    }
}

auto CreateDirectory(Path const & path) -> Result<void> { // mkdir (dirname)

    auto result = fs::CreateDirectory(path);
    if(not result)
    {
        return result.error();
    }
    return outcome_v2::success();
}

auto RemoveObject(Path const & path) -> Result<void> { // rm <filename/dirname>

    auto result = fs::Remove(path);
    if(not result)
    {
        return result.error();
    }
    return outcome_v2::success();
}

auto ChangeDirectory(Path const & path) -> Result<void> // cd <path>
{

    Result<DirectoryIterator> iteratorResult = MakeIterator(path);
    if (not iteratorResult) {
        return iteratorResult.error();
    }

    currentWorkingDirectory = path;
    return outcome_v2::success();
}

auto Touch(Path const & path) -> Result<void> // touch <filename>
{
    auto fileResult = Open(path, LFS_O_CREAT | LFS_O_WRONLY);
    if(not fileResult)
    {
        return fileResult.error();
    }
    return fileResult.value().Close();
}
