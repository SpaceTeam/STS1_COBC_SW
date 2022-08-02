#pragma once

#include <etl/string.h>

#include <string_view>


namespace cobc
{
/**
 * @brief The core implementation of the executable
 *
 * This class makes up the library part of the executable, which means that the
 * main logic is implemented here. This kind of separation makes it easy to
 * test the implementation for the executable, because the logic is nicely
 * separated from the command-line logic implemented in the main function.
 */
struct Library
{
    /**
     * @brief Simply initializes the name member to the name of the project
     */
    Library();

    static constexpr auto maxShortNameLength = 8;

    std::string_view name;
    etl::string<maxShortNameLength> shortName;
};
}
