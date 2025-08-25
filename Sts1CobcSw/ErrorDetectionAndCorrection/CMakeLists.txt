target_sources(Sts1CobcSw_ErrorDetectionAndCorrection PRIVATE ErrorDetectionAndCorrection.cpp)
target_link_libraries(Sts1CobcSw_ErrorDetectionAndCorrection PUBLIC Sts1CobcSw_Serial)
target_link_libraries(Sts1CobcSw_ErrorDetectionAndCorrection PRIVATE littlefs::littlefs)
if(BUILD_BOOTLOADER)
    target_compile_definitions(Sts1CobcSw_ErrorDetectionAndCorrection PUBLIC BUILD_BOOTLOADER)
else()
    target_link_libraries(
        Sts1CobcSw_ErrorDetectionAndCorrection PUBLIC rodos::rodos Sts1CobcSw_Utility
    )
endif()
