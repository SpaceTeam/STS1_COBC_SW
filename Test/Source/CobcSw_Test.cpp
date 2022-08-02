#include "Lib.hpp"


auto main() -> int
{
  auto const lib = cobc::Library{};

  return lib.name == "CobcSw" ? 0 : 1;
}
