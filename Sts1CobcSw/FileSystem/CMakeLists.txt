target_sources(Sts1CobcSw_FileSystem PRIVATE DirectoryIterator.cpp FileSystem.cpp File.cpp)
target_link_libraries(
    Sts1CobcSw_FileSystem PUBLIC etl::etl littlefs::littlefs Sts1CobcSw_Outcome Sts1CobcSw_Serial
                                 Sts1CobcSw_Vocabulary
)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_FileSystem PRIVATE LfsFlash.cpp)
    target_link_libraries(
        Sts1CobcSw_FileSystem PRIVATE rodos::rodos Sts1CobcSw_Periphery Sts1CobcSw_RodosTime
    )
else()
    target_sources(Sts1CobcSw_FileSystem PRIVATE LfsRam.cpp)
    target_link_libraries(Sts1CobcSw_FileSystem PRIVATE rodos::without_main_on_linux)
endif()
