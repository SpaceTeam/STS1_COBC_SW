#pragma once

namespace sts1cobcsw::bootloader::fram
{
auto SetCsPin() -> void;
auto ResetCsPin() -> void;
auto Initialize() -> void;
auto Reset() -> void;
auto ReadId() -> void;
auto Write(unsigned long address, char const * string, int size) -> void;
auto Read(unsigned long address, char * string, int size) -> void;
auto PersistentWariableRead(char32_t address, unsigned int blockSize) -> unsigned int;
auto PersistentWariableWrite(char32_t address, unsigned int data, unsigned int blockSize) -> void;
}
