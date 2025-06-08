#pragma once


namespace sts1cobcsw
{
// This function must be marked as [[noreturn]] to tell the compiler that it doesn't need to clean
// up the stack before jumping to the firmware. Without this, calling a function in RunFirmware()
// that does not get inlined, causes the compiler to generate stack clean-up code after we manually
// set the main stack pointer. This ultimately leads to loading from an invalid address and a hard
// fault.
[[noreturn]] auto RunFirmware() -> void;
}
