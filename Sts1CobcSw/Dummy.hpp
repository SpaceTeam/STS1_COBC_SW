#pragma once


#include <etl/string.h>


namespace sts1cobcsw
{
struct Dummy
{
    Dummy();

    static constexpr auto maxNameLength = 42;

    etl::string<maxNameLength> name = "";
};
}
