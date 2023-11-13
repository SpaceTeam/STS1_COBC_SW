target_sources(Sts1CobcSw_Utility PRIVATE Crc32.cpp Time.cpp)
# TODO: Break cyclic dependency with Sts1CobcSw_Serial (maybe by putting Byte to Utility?)
target_link_libraries(Sts1CobcSw_Utility PUBLIC rodos::rodos Sts1CobcSw_Serial)
