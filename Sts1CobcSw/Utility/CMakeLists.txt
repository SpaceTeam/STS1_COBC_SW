target_sources(Sts1CobcSw_Utility PRIVATE Crc32Software.cpp)
target_link_libraries(
    Sts1CobcSw_Utility PUBLIC etl::etl rodos::without_main_on_linux strong_type::strong_type
                              Sts1CobcSw_Serial
)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_Utility PRIVATE Crc32Hardware.cpp)
endif()
