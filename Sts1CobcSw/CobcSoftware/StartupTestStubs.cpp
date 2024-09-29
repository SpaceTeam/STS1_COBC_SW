#include <Sts1CobcSw/CobcSoftware/SpiStartupTestAndSupervisorThread.hpp>


namespace sts1cobcsw
{
// TODO: These functions must also set fram::framIsWorking, as well as "flashIsWorking" and
// "rfIsWorking" to true.
auto ResumeFlashStartupTestThread() -> void
{
    ResumeSpiStartupTestAndSupervisorThread();
}


auto ResumeFramEpsStartupTestThread() -> void
{
    ResumeSpiStartupTestAndSupervisorThread();
}


auto ResumeRfStartupTestThread() -> void
{
    ResumeSpiStartupTestAndSupervisorThread();
}
}
