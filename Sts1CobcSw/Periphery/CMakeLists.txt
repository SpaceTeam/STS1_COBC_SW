if(CMAKE_SYSTEM_NAME STREQUAL Generic)
	target_sources(Sts1CobcSw_Periphery PRIVATE Edu.cpp)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
	target_sources(Sts1CobcSw_Periphery PRIVATE EduMock.cpp)
endif()

target_link_libraries(Sts1CobcSw_Periphery PRIVATE rodos::rodos etl::etl Sts1CobcSw_Hal)
