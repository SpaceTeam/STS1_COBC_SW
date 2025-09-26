target_sources(Sts1CobcSw_FirmwareManagement PRIVATE FirmwareManagement.cpp)
if(BUILD_BOOTLOADER)
    target_compile_definitions(Sts1CobcSw_FirmwareManagement PUBLIC BUILD_BOOTLOADER)
endif()
target_link_libraries(
    Sts1CobcSw_FirmwareManagement PUBLIC strong_type::strong_type Sts1CobcSw_Outcome
                                         Sts1CobcSw_Serial Sts1CobcSw_Vocabulary
)
target_link_libraries(
    Sts1CobcSw_FirmwareManagement PRIVATE etl::etl Sts1CobcSw_ErrorDetectionAndCorrection
                                          Sts1CobcSw_Utility
)
if(NOT CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries(Sts1CobcSw_FirmwareManagement PRIVATE Sts1CobcSw_CmsisDevice)
endif()
