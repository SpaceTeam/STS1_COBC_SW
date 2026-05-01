#include <Sts1CobcSw/Edu/ProgramQueue.hpp>
#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#include <Sts1CobcSw/Firmware/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/FirmwareManagement/FirmwareManagement.hpp>
#include <Sts1CobcSw/Flash/Flash.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/RealTime/RealTime.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Vocabulary/Ids.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <cassert>
#include <cinttypes>  // IWYU pragma: keep
#include <cstdint>
#include <utility>

namespace sts1cobcsw
{
namespace
{
constexpr auto stackSize = 10'000U;
constexpr auto radiationStartupTime = 60 * s;

constexpr auto threadPeriod = 1 * s;
constexpr auto eduProgramInterval = 5 * min;
constexpr auto checkProgramInterval = 10 * min;

constexpr auto testProgramId = ProgramId(10);  // ToDo: what value
constexpr auto testProgramTimeout = 300;       // ToDo: what value

constexpr auto beaconInterval = 30 * s;
static_assert(checkProgramInterval
              > beaconInterval);  // if it is smaller we could miss sending a error
static_assert(eduProgramInterval > threadPeriod);
static_assert(checkProgramInterval > threadPeriod);

[[nodiscard]] auto IsFwIntact() -> bool;
[[nodiscard]] auto IsFramIntact() -> bool;
auto FillFram() -> void;
[[nodiscard]] auto IsFlashIntact() -> bool;
auto RepairFlash(lfs_block_t corruptedBlock) -> void;

class RadiationThread : public RODOS::StaticThread<stackSize>
{
public:
    RadiationThread() : StaticThread("RadiationThread", radiationThreadPriority)
    {}


private:
    void init() override
    {}


    void run() override  // NOLINT(readability-function-cognitive-complexity)
    {
        SuspendFor(totalStartupTestTimeout);  // Wait for the startup tests to complete
        DEBUG_PRINT("Starting radiation thread\n");
        auto framIntact = false;
        auto flashIntact = false;
        auto fwIntact = false;
        std::uint8_t memoryIntact = 0U;
        auto eduWaitCounter = INT64_MAX;
        auto checkWaitCounter = 0LL;
        SuspendFor(radiationStartupTime);  // Wait for the system to start
        DEBUG_PRINT("Start fram filling\n");
        FillFram();  // 3min on unfilled fram
        DEBUG_PRINT("Fram filling complete\n");
        auto loopStartTime = RodosTime(0);
        auto loopEndTime = RodosTime(0);

        while(true)
        {
            loopStartTime = CurrentRodosTime();
            if(eduWaitCounter * threadPeriod >= eduProgramInterval)
            {
                DEBUG_PRINT("Add edu radiation program\n");
                persistentVariables.Store<"eduProgramQueueIndex">(eduProgramQueueIndexResetValue);
                edu::programQueue.Clear();
                edu::programQueue.PushBack(
                    edu::ProgramQueueEntry{testProgramId, CurrentRealTime(), testProgramTimeout});
                ResumeEduProgramQueueThread();
                eduWaitCounter = 0;
            }

            if(checkWaitCounter * threadPeriod >= checkProgramInterval)
            {
                DEBUG_PRINT("Run radiation checks\n");
                // get all results then publish them with next beacon
                fwIntact = IsFwIntact();  // ~5sec
                DEBUG_PRINT("Firmware check complete\n");
                framIntact = IsFramIntact();  // ~1sec
                DEBUG_PRINT("Fram check complete\n");
                flashIntact = IsFlashIntact();  // 5min
                DEBUG_PRINT("Flash check complete\n");

                if(not fwIntact)
                {
                    DEBUG_PRINT("Firmware corrupted\n");
                }
                if(not framIntact)
                {
                    DEBUG_PRINT("Fram corrupted\n");
                }
                if(not flashIntact)
                {
                    DEBUG_PRINT("Flash corrupted\n");
                }

                memoryIntact = static_cast<uint8_t>((not framIntact ? 1U : 0U)
                                                    | ((not flashIntact ? 1U : 0U) << 1U)
                                                    | ((not fwIntact ? 1U : 0U) << 2U));
                memoryIntactTopic.publish(memoryIntact);
                checkWaitCounter = 0;
            }

            loopEndTime = CurrentRodosTime();
            auto timePassed = loopEndTime - loopStartTime;
            if(timePassed > threadPeriod)
            {
                eduWaitCounter += timePassed % threadPeriod;
                checkWaitCounter += timePassed % threadPeriod;
            }
            else
            {
                ++eduWaitCounter;
                ++checkWaitCounter;
            }
            SuspendFor(threadPeriod);
        }
    }
} radiationThread;


auto IsFwIntact() -> bool
{
#ifdef BUILD_FOR_USE_WITH_BOOTLOADER
    auto const partition1Address = fw::GetPartition(PartitionId::primary);
    auto fwPartition1Intact = fw::CheckFirmwareIntegrity(partition1Address.startAddress);

    auto const partition2Address = fw::GetPartition(PartitionId::secondary1);
    auto fwPartition2Intact = fw::CheckFirmwareIntegrity(partition2Address.startAddress);


    return fwPartition1Intact && fwPartition2Intact;
#else
    return true;
#endif
}


auto IsFramIntact() -> bool
{
    static constexpr auto framTimeout = 10 * ms;
    static constexpr auto chunkSize = 2 * 1024U;
    static constexpr auto startAddress = framSections.Get<"testMemory">().begin;
    static constexpr auto endAddress = framSections.Get<"testMemory">().end - fram::Size(chunkSize);

    static_assert(value_of(framSections.Get<"testMemory">().size) % chunkSize == 0);

    static constexpr auto fillerData = 0xAA_b;
    for(auto address = startAddress; address < endAddress; address += fram::Size(chunkSize))
    {
        auto data = std::array<Byte, chunkSize>{};

        fram::ReadFrom(address, Span(&data), framTimeout);

        bool allMatch = std::all_of(
            data.begin(), data.end(), [](Byte content) { return content == fillerData; });
        if(not allMatch)
        {
            // repair chunk
            data.fill(fillerData);
            RODOS::PRINTF("Fram corrupted at chunk: %lx\n",
                          static_cast<unsigned long>(value_of(address)));
            fram::WriteTo(address, Span(data), framTimeout);

            return false;
        }
    }
    return true;
}

auto FillFram() -> void
{
    // The first time we run this, the fram is not filled -> we need nChunks times to call
    // isFramIntact (only the first corrupt chunk found is fixed)
    static constexpr auto chunkSize = 2 * 1024U;
    static constexpr auto nChunks = value_of(framSections.Get<"testMemory">().size) / chunkSize;
    for(auto i = 0U; i < nChunks; i++)
    {
        if(IsFramIntact())
        {
            return;
        }
    }
}

auto IsFlashIntact() -> bool
{
    // we use the lfs read function, as it already implements a crc check for used pages and a
    // pattern for unused pages
    auto readFunction = sts1cobcsw::fs::lfsConfig.read;
    auto const sectorSize = 4 * 1024;
    auto const nSectors = sts1cobcsw::fs::lfsConfig.block_count;
    auto buffer = std::array<Byte, sectorSize>();

    for(lfs_block_t i = 0U; i < nSectors; i++)
    {
        int result = readFunction(
            &sts1cobcsw::fs::lfsConfig, i, 0, buffer.data(), sts1cobcsw::fs::lfsConfig.block_size);
        if(result == LFS_ERR_CORRUPT)
        {
            DEBUG_PRINT("Flash corrupted at block %lx\n", static_cast<unsigned long>(i));
            RepairFlash(i);
            return false;
        }
    }

    return true;
}

auto RepairFlash(lfs_block_t corruptedBlock) -> void
{
    auto readFunction = sts1cobcsw::fs::lfsConfig.read;
    auto progFunction = sts1cobcsw::fs::lfsConfig.prog;
    auto const sectorSize = 4 * 1024;
    auto const eraseValue = 0xFF_b;  // needs to match the eraseValue of LfsFlash.cpp
    auto buffer = flash::Page();

    for(lfs_block_t i = 0U; i < sectorSize; i += sts1cobcsw::fs::lfsConfig.read_size)
    {
        int result = readFunction(
            &sts1cobcsw::fs::lfsConfig, corruptedBlock, i, buffer.data(), flash::pageSize);
        if(result == LFS_ERR_CORRUPT)
        {
            buffer.fill(eraseValue);
            progFunction(
                &sts1cobcsw::fs::lfsConfig, corruptedBlock, i, buffer.data(), flash::pageSize);
            return;
        }
    }
}
}
}
