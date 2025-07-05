#pragma once

namespace sts1cobcsw::bootloader::fram
{
auto SetCsPin() -> void;
auto ResetCsPin() -> void;
auto Initialize() -> void;
auto Reset() -> void;
auto ReadId() -> void;
auto Write(unsigned long address, char const * string, int size) -> void;
auto Read(unsigned long address, char *stringPass, int size) -> void;
auto PersistentWariableRead(unsigned long address, unsigned long blockSize) -> unsigned int;
auto PersistentWariableWrite(unsigned int address, unsigned int data, unsigned long int blockSize) -> void;
}
