target_sources(
    Sts1CobcSw_Periphery PRIVATE Edu.cpp EduStructs.cpp Flash.cpp PersistentState.cpp Rf.cpp
)
target_link_libraries(
    Sts1CobcSw_Periphery PUBLIC rodos::rodos type_safe Sts1CobcSw_Hal Sts1CobcSw_Serial
)
target_link_libraries(Sts1CobcSw_Periphery PRIVATE Sts1CobcSw_Utility)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_Periphery PRIVATE Edu.cpp PersistentState.cpp Flash.cpp Fram.cpp)
    target_link_libraries(Sts1CobcSw_Periphery PUBLIC Sts1CobcSw_Hal)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_sources(Sts1CobcSw_Periphery PRIVATE EduMock.cpp)
    # TODO: Why does it compile without Sts1CobcSw_Hal even though Edu.hpp includes /Hal/GpioPin.hpp
    # and /Hal/IoNames.hpp? And why does it compile on in general when Edu has a member of type
    # RODOS::HAL_UART?
endif()
