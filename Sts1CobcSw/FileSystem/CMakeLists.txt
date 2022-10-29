target_sources(Sts1CobcSw_FileSystem PRIVATE FileSystem.cpp)
target_link_libraries(
    Sts1CobcSw_FileSystem PRIVATE littlefs::littlefs Sts1CobcSw_Periphery Sts1CobcSw_Serial
)
