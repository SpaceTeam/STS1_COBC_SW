#include <Tests/HardwareTests/Utility.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
auto Check(bool condition, std::string_view failMessage, std::string_view successMessage) -> void
{
    if(condition)
    {
        RODOS::PRINTF("%s", successMessage.data());
    }
    else
    {
        RODOS::PRINTF("%s", failMessage.data());
    }
}
}
