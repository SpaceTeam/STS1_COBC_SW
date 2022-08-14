target_sources(CobcSw_Test PRIVATE CobcSw_Test.cpp)
target_link_libraries(CobcSw_Test PRIVATE CobcSw_Lib Catch2::Catch2WithMain)
