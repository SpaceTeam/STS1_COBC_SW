target_sources(Sts1CobcSw_FirmwareManagement PRIVATE FirmwareManagement.cpp)
target_link_libraries(
    Sts1CobcSw_FirmwareManagement PUBLIC Sts1CobcSw_Outcome Sts1CobcSw_Serial Sts1CobcSw_Vocabulary
)
target_link_libraries(
    Sts1CobcSw_FirmwareManagement PRIVATE etl::etl Sts1CobcSw_ErrorDetectionAndCorrection
                                          Sts1CobcSw_Utility
)
if(NOT CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries(Sts1CobcSw_FirmwareManagement PRIVATE Sts1CobcSw_CmsisDevice)
endif()
