#pragma once


#include <Tests/CatchRodos/Vocabulary.hpp>

#include <source_location>


namespace sts1cobcsw
{
struct Assertion
{
    gsl::czstring macroName = "??";
    gsl::czstring expression = "??";
    gsl::czstring op = "??";
    std::source_location location = std::source_location::current();
};
}
