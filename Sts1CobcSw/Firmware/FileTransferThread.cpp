#include <Sts1CobcSw/Firmware/FileTransferThread.hpp>

#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/Mailbox/Mailbox.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Vocabulary/Ids.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
namespace
{
constexpr auto stackSize = 2000U;


auto SendFile(FileTransferInfo const & fileTransferInfo) -> void;
auto ReceiveFile(FileTransferInfo const & fileTransferInfo) -> void;
auto ReceiveFirmware(FileTransferInfo const & fileTransferInfo) -> void;


class FileTransferThread : public RODOS::StaticThread<stackSize>
{
public:
    FileTransferThread() : StaticThread("FileTransferThread", fileTransferThreadPriority)
    {}


private:
    auto run() -> void override
    {
        SuspendFor(totalStartupTestTimeout);  // Wait for the startup tests to complete
        DEBUG_PRINT("Starting file transfer thread\n");
        while(true)
        {
            // There cannot be a timeout error if we wait until the end of time
            (void)fileTransferInfoMailbox.SuspendUntilFullOr(endOfTime);
            if(fileTransferInfoMailbox.IsEmpty())
            {
                continue;
            }
            auto fileTransferInfo = fileTransferInfoMailbox.Get().value();
            if(fileTransferInfo.sourceEntityId == cubeSatEntityId)
            {
                DEBUG_PRINT("Sending file to ground station: '%s' -> '%s'\n",
                            fileTransferInfo.sourcePath.c_str(),
                            fileTransferInfo.destinationPath.c_str());
                SendFile(fileTransferInfo);
            }
            else
            {
                if(fileTransferInfo.fileIsFirmware)
                {
                    DEBUG_PRINT("Receiving firmware from ground station: '%s' -> FW partition %s\n",
                                fileTransferInfo.sourcePath.c_str(),
                                ToCZString(fileTransferInfo.destinationPartitionId));
                    ReceiveFirmware(fileTransferInfo);
                }
                else
                {
                    DEBUG_PRINT("Receiving file from ground station: '%s' -> '%s'\n",
                                fileTransferInfo.sourcePath.c_str(),
                                fileTransferInfo.destinationPath.c_str());
                    ReceiveFile(fileTransferInfo);
                }
            }
        }
    }
} fileTransferThread;
}


auto ResumeFileTransferThread() -> void
{
    fileTransferThread.resume();
}


namespace
{
auto SendFile(FileTransferInfo const & fileTransferInfo) -> void
{
    (void)fileTransferInfo;
}


auto ReceiveFile(FileTransferInfo const & fileTransferInfo) -> void
{
    (void)fileTransferInfo;
}


auto ReceiveFirmware(FileTransferInfo const & fileTransferInfo) -> void
{
    (void)fileTransferInfo;
}
}
}
