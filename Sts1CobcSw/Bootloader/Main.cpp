#include <Sts1CobcSw/Bootloader/Leds.hpp>
#include <Sts1CobcSw/Bootloader/RunFirmware.hpp>
#include <Sts1CobcSw/Bootloader/UciUart.hpp>


auto main() -> int
{
    sts1cobcsw::leds::Initialize();
    sts1cobcsw::leds::TurnOn();
    sts1cobcsw::uciuart::Initialize();
    sts1cobcsw::uciuart::Write("Hello from the bootloader!\n");
    sts1cobcsw::RunFirmware();
}
