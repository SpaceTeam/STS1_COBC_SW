#pragma once


#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>


namespace sts1cobcsw
{
struct FileTransferInfo
{
    fs::Path sourcePath;
    fs::Path destinationPath;
    EntityId sourceEntityId;
    EntityId destinationEntityId;
};


auto ResumeFileTransferThread() -> void;
}
