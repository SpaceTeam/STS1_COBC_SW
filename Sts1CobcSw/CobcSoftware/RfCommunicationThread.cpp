#include <Sts1CobcSw/CobcSoftware/RfCommunicationThread.hpp>

#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
namespace
{
// TODO: Get a better estimation for the required stack size.
constexpr auto stackSize = 1000;


class RfCommunicationThread : public RODOS::StaticThread<stackSize>
{
public:
    RfCommunicationThread() : StaticThread("RfCommunicationThread", rfCommunicationThreadPriority)
    {}


private:
    void init() override
    {}


    void run() override
    {}
} rfCommunicationThread;
}


auto ResumeRfCommunicationThread() -> void
{
    rfCommunicationThread.resume();
}
}
