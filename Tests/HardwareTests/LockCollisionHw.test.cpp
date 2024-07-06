#include <Sts1CobcSw/FileSystem/LfsStorageDevice.hpp>

#include <littlefs/lfs.h>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
using RODOS::PRINTF;

bool volatile formatDone = false;
auto lfs = lfs_t{};


auto Check(char const * threadName, int errorCode, int expectedErrorCode) -> bool
{
    if(errorCode != expectedErrorCode)
    {
        PRINTF("%s: Error code: %d, Expected error code: %d\n",
               threadName,
               errorCode,
               expectedErrorCode);
        return false;
    }
    return true;
}


class LockingThread : public RODOS::StaticThread<>
{
    char const * directoryPath;
    char const * filePath;
    int fileContent;

public:
    explicit LockingThread(char const * threadName,
                           uint32_t threadPriotiry,
                           char const * directoryPath,
                           char const * filePath,
                           int fileContent)
        : StaticThread(threadName, threadPriotiry),
          directoryPath{directoryPath},
          filePath{filePath},
          fileContent{fileContent}
    {
    }


private:
    void init() override
    {
    }


    void run() override
    {
        auto errorCode = -1;
        constexpr auto lockBusyError = -99;
        constexpr auto retryDelay = 1 * RODOS::MICROSECONDS;
        if(strcmp(this->name, "Thread 1") == 0)
        {
            sts1cobcsw::fs::Initialize();

            PRINTF("%s: Try format\n", this->name);
            while((errorCode = lfs_format(&lfs, &sts1cobcsw::fs::lfsConfig)) == lockBusyError)
            {
                RODOS::AT(RODOS::NOW() + retryDelay);
            }
            assert(Check(this->name, errorCode, 0) and "format");
            PRINTF("%s: Format success\n", this->name);


            formatDone = true;
        }

        while(not formatDone)
        {
            // A high priority thread will block a lower priority thread trying to set up and format
            // when just using yield. Either the thread setting up the file system needs to be high
            // priority, or must be given time by sleeping.
            RODOS::AT(RODOS::NOW() + 1 * RODOS::MILLISECONDS);
            // yield();
        }

        PRINTF("%s: Try mount\n", this->name);
        while((errorCode = lfs_mount(&lfs, &sts1cobcsw::fs::lfsConfig)) == lockBusyError)
        {
            RODOS::AT(RODOS::NOW() + retryDelay);
            // yield();
        }
        assert(Check(this->name, errorCode, 0) and "mount");
        PRINTF("%s: Mount success\n", this->name);

        PRINTF("%s: Try making directory %s\n", this->name, directoryPath);
        while((errorCode = lfs_mkdir(&lfs, directoryPath)) == lockBusyError)
        {
            RODOS::AT(RODOS::NOW() + retryDelay);
            // yield();
        }
        assert(Check(this->name, errorCode, 0) and "mkdir");
        PRINTF("%s: Making directory %s success\n", this->name, directoryPath);

        PRINTF("%s: Try opening file %s\n", this->name, filePath);
        auto file = lfs_file_t{};
        while((errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_WRONLY | LFS_O_CREAT))
              == lockBusyError)
        {
            RODOS::AT(RODOS::NOW() + retryDelay);
            // yield();
        }
        assert(Check(this->name, errorCode, 0) and "file_open");
        PRINTF("%s: Opening file %s success\n", this->name, filePath);

        PRINTF("%s: Try writing %i\n", this->name, fileContent);
        while((errorCode = lfs_file_write(&lfs, &file, &fileContent, sizeof(fileContent)))
              == lockBusyError)
        {
            RODOS::AT(RODOS::NOW() + retryDelay);
            // yield();
        }
        assert(Check(this->name, errorCode, sizeof(fileContent)) and "file_write");
        PRINTF("%s: Writing %i success\n", this->name, fileContent);

        PRINTF("%s: Try closing file\n", this->name);
        while((errorCode = lfs_file_close(&lfs, &file)) == lockBusyError)
        {
            RODOS::AT(RODOS::NOW() + retryDelay);
            // yield();
        }
        assert(Check(this->name, errorCode, 0) and "file_close");
        PRINTF("%s: Closing file success\n", this->name);

        PRINTF("%s: Try unmounting\n", this->name);
        while((errorCode = lfs_unmount(&lfs)) == lockBusyError)
        {
            RODOS::AT(RODOS::NOW() + retryDelay);
            // yield();
        }
        assert(Check(this->name, errorCode, 0) and "unmount");
        PRINTF("%s: Unmounting success\n", this->name);

        PRINTF("%s: Try second mount\n", this->name);
        while((errorCode = lfs_mount(&lfs, &sts1cobcsw::fs::lfsConfig)) == lockBusyError)
        {
            RODOS::AT(RODOS::NOW() + retryDelay);
            // yield();
        }
        assert(Check(this->name, errorCode, 0) and "mount");
        PRINTF("%s: Second mount success\n", this->name);

        PRINTF("%s: Try opening file %s again\n", this->name, filePath);
        while((errorCode = lfs_file_open(&lfs, &file, filePath, LFS_O_RDONLY)) == lockBusyError)
        {
            RODOS::AT(RODOS::NOW() + retryDelay);
            // yield();
        }
        assert(Check(this->name, errorCode, 0) and "file_open");
        PRINTF("%s: Opening file %s again success\n", this->name, filePath);

        PRINTF("%s: Try reading file %s\n", this->name, filePath);
        int readNumber = 0;
        while((errorCode = lfs_file_read(&lfs, &file, &readNumber, sizeof(fileContent)))
              == lockBusyError)
        {
            RODOS::AT(RODOS::NOW() + retryDelay);
            // yield();
        }
        assert(Check(this->name, errorCode, sizeof(fileContent)) and "file_read");
        assert(readNumber == fileContent);
        PRINTF("%s: Reading file %s success: Read %i\n", this->name, filePath, readNumber);

        PRINTF("%s: Try closing file again\n", this->name);
        while((errorCode = lfs_file_close(&lfs, &file)) == lockBusyError)
        {
            RODOS::AT(RODOS::NOW() + retryDelay);
            // yield();
        }
        assert(Check(this->name, errorCode, 0) and "file_close");
        PRINTF("%s: Closing file again success\n", this->name);
        RODOS::PRINTF("%s: Done\n", this->name);
        RODOS::AT(RODOS::END_OF_TIME);
    }
};
auto thread1 = LockingThread("Thread 1", 100, "Folder1", "Folder1/File1", 111);
auto thread2 = LockingThread("Thread 2", 100, "Folder2", "Folder2/File2", 222);
auto thread3 = LockingThread("Thread 3", 200, "Folder3", "Folder3/File3", 333);
}
