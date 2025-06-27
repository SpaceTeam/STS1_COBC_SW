#pragma once

namespace sts1cobcsw::ucispi
{
auto Initialize() -> void;
auto Reset() -> void;
auto Write(unsigned char character) -> void;
auto Read(char *character) -> void;
auto SetCsPin() -> void;
auto ResetCsPin() -> void;
auto FramWrite(char32_t address, char const * string, int size) -> void;
auto FramRead(char32_t address, char *string, int size) -> void;
auto PersistentWariableRead(char32_t address) -> unsigned int;
auto PersistentWariableWrite(char32_t address, unsigned int data) -> void;
}
