#include <Sts1CobcSw/Bootloader/BusyWait.hpp>


namespace sts1cobcsw
{
auto BusyWaitUs(int duration) -> void
{
    for(int i = 0; i < duration; ++i)
    {
        // NOLINTBEGIN(*no-assembler)
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
#ifdef NDEBUG
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
#endif
        // NOLINTEND(*no-assembler)
    }
}
}
