target_sources(CobcSw_Lib PRIVATE Lib.cpp)
target_include_directories(
    CobcSw_Lib ${warning_guard} PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Source>"
)
target_link_libraries(CobcSw_Lib PUBLIC etl)

target_sources(CobcSw_HelloWorld PRIVATE HelloWorld.cpp)
target_link_libraries(CobcSw_HelloWorld PRIVATE CobcSw_Lib rodos::rodos)
