add_library(External_Blake2s STATIC)
add_library(External::Blake2s ALIAS External_Blake2s)
target_sources(External_Blake2s PRIVATE BLAKE2s.cpp Crypto.cpp Hash.cpp)
set_target_properties(External_Blake2s PROPERTIES CXX_INCLUDE_WHAT_YOU_USE "")
