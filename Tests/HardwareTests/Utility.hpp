#pragma once


#include <string_view>


namespace sts1cobcsw
{
auto Check(bool condition,
           std::string_view failMessage = " -> Failed\n",
           std::string_view successMessage = " -> Passed\n") -> void;
}
