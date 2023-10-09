#include <Sts1CobcSw/Dummy.hpp>

#include <ringbuffer.h>

#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <cstdint>


std::uint32_t printfMask = 0;


namespace sts1cobcsw
{

enum Status
{
    notStarted,
    running,
    done
};

struct Entry
{
    std::uint32_t id;
    Status status;
};

auto StatusToString(Status status) -> std::string
{
    switch(status)
    {
        case notStarted:
            return {"notStarted"};
        case running:
            return {"running"};
        case done:
            return {"done"};
        default:
            break;
    }
}

// Defined a global variable for testing purpose
RODOS::RingBuffer<Entry, 10> buffer;

auto UpdateRingBuffer(Status status, std::uint32_t id, Status newStatus) -> void
{
    auto isLookedForEntry = [&](Entry & entry)
    { return (entry.id == id and entry.status == status); };

    auto count = 0;
    for(int i = 0; i < buffer.getLen(); ++i)
    {
        if(buffer.vals[i].id == id and buffer.vals[i].status == status)
        {
            count++;
        }
    }

    assert(count == 1 and "More than one program and status history match founds");

    for(int i = 0; i < buffer.getLen(); ++i)
    {
        if(buffer.vals[i].id == id and buffer.vals[i].status == status)
        {
            buffer.vals[i].status = newStatus;
        }
    }
}


void PrintBuffer()
{
    for(int i = 0; i < buffer.occupiedCnt; ++i)
    {
        RODOS::PRINTF("Vals[%d] = .id(%d), .status(%s)\n",
                      i,
                      buffer.vals[i].id,
                      StatusToString(buffer.vals[i].status).c_str());
    }
}

class HelloDummy : public RODOS::StaticThread<>
{
    void run() override
    {
        printfMask = 1;
        buffer.put(Entry{.id = 1, .status = Status::notStarted});
        buffer.put(Entry{.id = 2, .status = Status::notStarted});
        buffer.put(Entry{.id = 3, .status = Status::notStarted});
        buffer.put(Entry{.id = 4, .status = Status::notStarted});
        buffer.put(Entry{.id = 5, .status = Status::notStarted});

        RODOS::PRINTF("Occupied count : %d\n", buffer.occupiedCnt);

        // Print RingBuffer
        PrintBuffer();

        // Start programs 1 , 3 and 5
        UpdateRingBuffer(Status::notStarted, 1, Status::running);
        UpdateRingBuffer(Status::notStarted, 3, Status::running);

        // Print RingBuffer
        PrintBuffer();

        RODOS::hwResetAndReboot();
    }
} helloDummy;
}
