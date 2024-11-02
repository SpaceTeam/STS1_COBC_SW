#include <Sts1CobcSw/CobcSoftware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>


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
