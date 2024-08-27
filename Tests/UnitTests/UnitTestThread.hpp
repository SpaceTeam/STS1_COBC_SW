#pragma once


#include <source_location>


auto RunUnitTest() -> void;
auto Require(bool condition, std::source_location location = std::source_location::current())
    -> void;
