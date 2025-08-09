#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/FileSystem/DirectoryIterator.hpp>
#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
namespace
{
using RODOS::PRINTF;


constexpr auto stackSize = 5000U;


auto PrintDirectoryEntries(fs::DirectoryIterator const & iterator) -> void;


class ListEduFiles : public RODOS::StaticThread<stackSize>
{
public:
    ListEduFiles() : StaticThread("ListEduFiles")
    {}

private:
    void run() override
    {
        PRINTF("\n");
        PRINTF("Listing all EDU files ...\n\n");
        fram::Initialize();
        fs::Initialize();
        auto mountResult = fs::Mount();
        if(mountResult.has_error())
        {
            PRINTF("Mounting file system failed: %s\n", ToCZString(mountResult.error()));
            return;
        }
        auto makeIteratorResult = fs::MakeIterator(edu::programsDirectory);
        if(makeIteratorResult.has_error())
        {
            PRINTF("Creating iterator for programs directory failed: %s\n",
                   ToCZString(makeIteratorResult.error()));
            return;
        }
        PRINTF("Programs:\n");
        PrintDirectoryEntries(makeIteratorResult.value());
        makeIteratorResult = fs::MakeIterator(edu::resultsDirectory);
        if(makeIteratorResult.has_error())
        {
            PRINTF("Creating iterator for results directory failed: %s\n",
                   ToCZString(makeIteratorResult.error()));
            return;
        }
        PRINTF("Results:\n");
        PrintDirectoryEntries(makeIteratorResult.value());
    }
} listEduFilesThread;


auto PrintDirectoryEntries(fs::DirectoryIterator const & iterator) -> void
{
    for(auto && entryResult : iterator)
    {
        if(entryResult.has_error())
        {
            PRINTF("Error reading entry: %s\n", ToCZString(entryResult.error()));
            continue;
        }
        auto & entry = entryResult.value();
        if(entry.type == fs::EntryType::file)
        {
            PRINTF("  %s (%d B)\n", entry.name.c_str(), static_cast<int>(entry.size));
        }
        else if(entry.type == fs::EntryType::directory)
        {
            PRINTF("  %s\n", entry.name.c_str());
        }
    }
}
}
}
