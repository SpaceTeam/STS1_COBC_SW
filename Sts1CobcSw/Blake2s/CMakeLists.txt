add_subdirectory(External)

target_sources(Sts1CobcSw_Blake2s PRIVATE Blake2s.cpp)
target_link_libraries(Sts1CobcSw_Blake2s PUBLIC Sts1CobcSw_Serial)
target_link_libraries(Sts1CobcSw_Blake2s PRIVATE External::Blake2s)
