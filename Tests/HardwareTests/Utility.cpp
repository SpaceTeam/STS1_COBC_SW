#include <Tests/HardwareTests/Utility.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
auto Check(bool condition, std::string_view failMessage, std::string_view successMessage) -> void
{
    if(condition)
    {
        RODOS::PRINTF("%s", data(successMessage));
    }
    else
    {
        RODOS::PRINTF("%s", data(failMessage));
    }
}
}
