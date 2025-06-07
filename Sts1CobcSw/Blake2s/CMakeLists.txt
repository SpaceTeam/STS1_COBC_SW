target_sources(Sts1CobcSw_Blake2s PRIVATE BLAKE2s.cpp Blake2sWrapper.cpp Crypto.cpp Hash.cpp)
target_link_libraries(Sts1CobcSw_Blake2s PUBLIC Sts1CobcSw_Serial)
