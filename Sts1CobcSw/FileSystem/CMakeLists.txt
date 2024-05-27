target_sources(Sts1CobcSw_FileSystem PRIVATE LfsWrapper.cpp)
target_link_libraries(Sts1CobcSw_FileSystem PUBLIC littlefs::littlefs Sts1CobcSw_Outcome)
target_link_libraries(Sts1CobcSw_FileSystem PRIVATE Sts1CobcSw_Serial)
target_compile_definitions(Sts1CobcSw_FileSystem PUBLIC LFS_THREADSAFE)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_FileSystem PRIVATE FileSystem.cpp LfsFlash.cpp)
    target_link_libraries(Sts1CobcSw_FileSystem PRIVATE Sts1CobcSw_Periphery)
else()
    target_sources(Sts1CobcSw_FileSystem PRIVATE LfsRam.cpp)
    target_sources(Sts1CobcSw_FileSystem_NoRodos PRIVATE LfsRam.cpp)
    target_sources(Sts1CobcSw_FileSystem PRIVATE LfsWrapper.cpp)
    target_link_libraries(
        Sts1CobcSw_FileSystem_NoRodos PUBLIC littlefs::littlefs Sts1CobcSw_Outcome
    )
    target_link_libraries(Sts1CobcSw_FileSystem_NoRodos PRIVATE Sts1CobcSw_Serial)
    target_compile_definitions(Sts1CobcSw_FileSystem_NoRodos PRIVATE NO_RODOS LFS_THREADSAFE)
endif()
