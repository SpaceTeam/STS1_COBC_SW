#pragma once

namespace sts1cobcsw::ucispi
{
auto Initialize() -> void;
auto Reset() -> void;
auto Write(unsigned char character) -> void;
auto Read(char *character) -> void;
auto SetCsPin() -> void;
auto ResetCsPin() -> void;
auto FramInitialize()->void;
auto FramReset()->void;
auto FramReadId() -> void;
auto FramWrite(unsigned long address, char const * string, int size) -> void;
auto FramRead(unsigned long address, char *string, int size) -> void;
auto PersistentWariableRead(char32_t address, unsigned int blockSize) -> unsigned int;
auto PersistentWariableWrite(char32_t address, unsigned int data, unsigned int blockSize) -> void;
}
