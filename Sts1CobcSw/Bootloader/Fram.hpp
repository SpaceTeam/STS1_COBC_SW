#pragma once

namespace sts1cobcsw::bootloader::fram
{
auto SetCsPin() -> void;
auto ResetCsPin() -> void;
auto Initialize() -> void;
auto Reset() -> void;
auto ReadId() -> void;
auto Write(unsigned long address, char const * string, int size) -> void;
auto Read(unsigned long address, char *string, int size) -> void;
auto PersistentWariableRead(unsigned long address, unsigned long blockSize) -> unsigned int;
auto PersistentWariableWrite(unsigned long address, unsigned int data, unsigned long blockSize) -> void;
}
