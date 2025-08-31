#pragma once


#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/FirmwareManagement/FirmwareManagement.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>


namespace sts1cobcsw
{
struct FileTransferInfo
{
    EntityId sourceEntityId;
    EntityId destinationEntityId;
    bool fileIsFirmware = false;
    PartitionId destinationPartitionId = PartitionId{};
    fs::Path sourcePath;
    fs::Path destinationPath;
};


auto ResumeFileTransferThread() -> void;
}
