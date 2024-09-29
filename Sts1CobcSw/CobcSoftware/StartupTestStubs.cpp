#include <Sts1CobcSw/CobcSoftware/SpiStartupTestAndSupervisorThread.hpp>


namespace sts1cobcsw
{
auto ResumeFlashStartupTestThread() -> void
{
    persistentVariables.template Store<"flashIsWorking">(true);
    ResumeSpiStartupTestAndSupervisorThread();
}


auto ResumeFramEpsStartupTestThread() -> void
{
    fram::framIsWorking.Store(true);
    persistentVariables.template Store<"epsIsWorking">(true);
    ResumeSpiStartupTestAndSupervisorThread();
}


auto ResumeRfStartupTestThread() -> void
{
    persistentVariables.template Store<"rfIsWorking">(true);
    ResumeSpiStartupTestAndSupervisorThread();
}
}
