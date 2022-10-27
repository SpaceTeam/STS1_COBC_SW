#include <Sts1CobcSw/EduProgramQueueThread.hpp>

#include <rodos.h>


uint32_t printfMask = 0;


// TODO: Fix all the errors and turn it into a useful test
namespace sts1cobcsw
{
class HelloDummy : public StaticThread<>
{
    void run() override
    {
        printfMask = 1;

        uint16_t progID = 2;
        uint16_t queueID = 1;
        // TODO: put some real time here, this is from dataflow game
        constexpr uint64_t startTime = 200;
        constexpr uint16_t timeout = 42;

        QueueEntry queueEntry = std::make_tuple(progID, queueID, startTime, timeout);

        std::array<QueueEntry, eduProgramQueueSize> eduQueue;
        eduQueue[0] = queueEntry;

        PRINTF("Element 0 in entry 0 is : %d\n", std::get<0>(eduQueue.at(0)));
        PRINTF("Element 1 in entry 0 is : %d\n", std::get<1>(eduQueue.at(0)));
        PRINTF("Element 2 in entry 0 is : %d\n", std::get<2>(eduQueue.at(0)));
        PRINTF("Element 3 in entry 0 is : %d\n", std::get<3>(eduQueue.at(0)));

        hwResetAndReboot();
    }
};

auto const helloDummy = HelloDummy();
}
