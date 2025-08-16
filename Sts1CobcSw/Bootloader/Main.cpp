#include <Sts1CobcSw/Bootloader/Print.hpp>
#include <Sts1CobcSw/Bootloader/RunFirmware.hpp>
#ifdef ENABLE_DEBUG_PRINT
    #include <Sts1CobcSw/Bootloader/Leds.hpp>
    #include <Sts1CobcSw/Bootloader/UciUart.hpp>
#endif


auto main() -> int
{
#ifdef ENABLE_DEBUG_PRINT
    sts1cobcsw::leds::Initialize();
    sts1cobcsw::leds::TurnOn();
    sts1cobcsw::uciuart::Initialize();
#endif
    DEBUG_PRINT("Hello from the bootloader!\n");
    sts1cobcsw::RunFirmware();
}
