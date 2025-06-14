add_library(Sts1CobcSwTests_HardwareSetupWithoutWatchdog STATIC)
add_library(
    Sts1CobcSwTests::HardwareSetupWithoutWatchdog ALIAS
    Sts1CobcSwTests_HardwareSetupWithoutWatchdog
)
target_sources(Sts1CobcSwTests_HardwareSetupWithoutWatchdog PRIVATE RfLatchupProtection.cpp)
target_include_directories(
    Sts1CobcSwTests_HardwareSetupWithoutWatchdog PUBLIC "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>"
)
if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSwTests_HardwareSetupWithoutWatchdog INTERFACE HardwareSetupThread.cpp)
    target_link_libraries(
        Sts1CobcSwTests_HardwareSetupWithoutWatchdog PUBLIC rodos::rodos Sts1CobcSw_Hal
    )
endif()

add_library(Sts1CobcSwTests_HardwareSetup INTERFACE)
add_library(Sts1CobcSwTests::HardwareSetup ALIAS Sts1CobcSwTests_HardwareSetup)
target_link_libraries(
    Sts1CobcSwTests_HardwareSetup INTERFACE Sts1CobcSwTests::HardwareSetupWithoutWatchdog
)
if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_compile_definitions(Sts1CobcSwTests_HardwareSetup INTERFACE USE_WATCHDOG)
    target_link_libraries(Sts1CobcSwTests_HardwareSetup INTERFACE Sts1CobcSw_WatchdogTimers)
endif()
