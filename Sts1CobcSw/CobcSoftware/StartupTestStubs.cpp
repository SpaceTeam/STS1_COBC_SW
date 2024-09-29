#include <Sts1CobcSw/CobcSoftware/SpiStartupTestAndSupervisorThread.hpp>


namespace sts1cobcsw
{
// TODO: These functions must also set fram::framIsWorking, as well as "flashIsWorking" and
// "rfIsWorking" to true.
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
