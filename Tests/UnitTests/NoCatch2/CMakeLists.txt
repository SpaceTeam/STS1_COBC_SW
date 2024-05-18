# No Catch2, since RODOS (main) is needed
add_program(LfsRamCustomSemaphore LfsRamCustomSemaphore.test.cpp)
target_link_libraries(Sts1CobcSwTests_LfsRamCustomSemaphore PRIVATE rodos::rodos Sts1CobcSw_FileSystem)

add_program(LockCollision LockCollision.test.cpp)
target_link_libraries(Sts1CobcSwTests_LockCollision PRIVATE rodos::rodos Sts1CobcSw_FileSystem)
