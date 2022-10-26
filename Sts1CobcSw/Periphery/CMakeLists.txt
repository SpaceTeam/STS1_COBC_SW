<<<<<<< HEAD
if(CMAKE_SYSTEM_NAME STREQUAL Generic)
	target_sources(Sts1CobcSw_Periphery PRIVATE Edu.cpp EduStructs.cpp PersistentState.cpp)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
	target_sources(Sts1CobcSw_Periphery PRIVATE EduMock.cpp EduStructs.cpp PersistentState.cpp)
endif()

target_link_libraries(Sts1CobcSw_Periphery PUBLIC rodos::rodos Sts1CobcSw_Hal Sts1CobcSw_Serial)
target_link_libraries(Sts1CobcSw_Periphery PRIVATE Sts1CobcSw_Utility type_safe)
