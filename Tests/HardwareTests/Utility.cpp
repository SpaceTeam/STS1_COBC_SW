#include <Tests/HardwareTests/Utility.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
auto allChecksPassed = true;


auto Check(bool condition, std::string_view failMessage, std::string_view successMessage) -> void
{
    if(condition)
    {
        RODOS::PRINTF("%s", successMessage.data());
    }
    else
    {
        allChecksPassed = false;
        RODOS::PRINTF("%s", failMessage.data());
    }
}


auto AllChecksPassed() -> bool
{
    return allChecksPassed;
}
}
