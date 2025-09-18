#pragma once

#include <Sts1CobcSw/Serial/Byte.hpp>

#include <etl/vector.h>

#include <cstddef>


namespace sts1cobcsw
{
// Precondition: dataField->available() >= sizeIncrease
auto IncreaseSize(etl::ivector<Byte> * dataField, std::size_t sizeIncrease) -> std::size_t;
}
