#include <Sts1CobcSw/CobcSoftware/SpiStartupTestAndSupervisorThread.hpp>


namespace sts1cobcsw
{
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
