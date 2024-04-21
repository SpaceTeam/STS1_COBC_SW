#pragma once


#include <Sts1CobcSw/FileSystem/ErrorsAndResult.hpp>


namespace sts1cobcsw::fs
{
[[nodiscard]] auto Mount() -> Result<void>;
}
