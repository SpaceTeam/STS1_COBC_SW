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


auto SendFile(FileTransferMetadata const & fileTransferMetadata) -> void;
auto ReceiveFile(FileTransferMetadata const & fileTransferMetadata) -> void;
auto ReceiveFirmware(FileTransferMetadata const & fileTransferMetadata) -> void;


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
            (void)fileTransferMetadataMailbox.SuspendUntilFullOr(endOfTime);
            if(fileTransferMetadataMailbox.IsEmpty())
            {
                continue;
            }
            auto fileTransferMetadata = fileTransferMetadataMailbox.Get().value();
            if(fileTransferMetadata.sourceEntityId == cubeSatEntityId)
            {
                DEBUG_PRINT("Sending file to ground station: '%s' -> '%s'\n",
                            fileTransferMetadata.sourcePath.c_str(),
                            fileTransferMetadata.destinationPath.c_str());
                SendFile(fileTransferMetadata);
            }
            else
            {
                if(fileTransferMetadata.fileIsFirmware)
                {
                    DEBUG_PRINT("Receiving firmware from ground station: '%s' -> FW partition %s\n",
                                fileTransferMetadata.sourcePath.c_str(),
                                ToCZString(fileTransferMetadata.destinationPartitionId));
                    ReceiveFirmware(fileTransferMetadata);
                }
                else
                {
                    DEBUG_PRINT("Receiving file from ground station: '%s' -> '%s'\n",
                                fileTransferMetadata.sourcePath.c_str(),
                                fileTransferMetadata.destinationPath.c_str());
                    ReceiveFile(fileTransferMetadata);
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
auto SendFile(FileTransferMetadata const & fileTransferMetadata) -> void
{
    (void)fileTransferMetadata;
}


auto ReceiveFile(FileTransferMetadata const & fileTransferMetadata) -> void
{
    (void)fileTransferMetadata;
}


auto ReceiveFirmware(FileTransferMetadata const & fileTransferMetadata) -> void
{
    (void)fileTransferMetadata;
}
}
}
