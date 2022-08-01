#include "lib.hpp"

auto main() -> int
{
  auto const lib = library {};

  return lib.name == "CobcSw" ? 0 : 1;
}
