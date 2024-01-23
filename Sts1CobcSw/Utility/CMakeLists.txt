target_sources(Sts1CobcSw_Utility PRIVATE Time.cpp)
target_link_libraries(Sts1CobcSw_Utility PUBLIC etl::etl rodos::rodos)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_Utility PRIVATE Crc32.cpp)
endif()
