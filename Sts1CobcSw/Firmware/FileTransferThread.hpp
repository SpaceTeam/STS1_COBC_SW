#pragma once


#include <Sts1CobcSw/FileSystem/FileSystem.hpp>


namespace sts1cobcsw
{
struct FileTransferInfo
{
    fs::Path sourcePath;
    fs::Path destinationPath;
};


auto ResumeFileTransferThread() -> void;
}
