target_sources(Sts1CobcSw_FileSystem PRIVATE LfsWrapper.cpp)
target_link_libraries(
    Sts1CobcSw_FileSystem PUBLIC etl::etl littlefs::littlefs Sts1CobcSw_Outcome
                                 Sts1CobcSw_ProgramId Sts1CobcSw_Serial
)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_FileSystem PRIVATE FileSystem.cpp LfsFlash.cpp)
    target_link_libraries(Sts1CobcSw_FileSystem PRIVATE rodos::rodos Sts1CobcSw_Periphery)
else()
    target_sources(Sts1CobcSw_FileSystem PRIVATE LfsRam.cpp)
endif()
