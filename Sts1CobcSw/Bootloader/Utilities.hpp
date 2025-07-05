#pragma once

namespace sts1cobcsw::bootloader::utilities
{
auto PrintHexString(char const * string, int stringLenght) -> void;
auto PrintBinString(char const * string, int stringLength) -> void;
auto PrintDecString(char const * string, int stringLength) -> void;
}