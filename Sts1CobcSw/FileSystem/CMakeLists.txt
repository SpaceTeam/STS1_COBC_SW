target_sources(Sts1CobcSw_FileSystem PRIVATE DirectoryIterator.cpp FileSystem.cpp File.cpp)
target_link_libraries(
    Sts1CobcSw_FileSystem PUBLIC etl::etl littlefs::littlefs Sts1CobcSw_Outcome Sts1CobcSw_Serial
                                 Sts1CobcSw_Vocabulary Sts1CobcSw_FramSections
)
target_link_libraries(Sts1CobcSw_FileSystem PRIVATE rodos::rodos)
if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_FileSystem PRIVATE LfsFlash.cpp)
    target_link_libraries(
        Sts1CobcSw_FileSystem PRIVATE strong_type::strong_type Sts1CobcSw_Flash
                                      Sts1CobcSw_Vocabulary
    )
else()
    target_sources(Sts1CobcSw_FileSystem PRIVATE LfsRam.cpp)
endif()
