target_sources(Sts1CobcSw_Hal PRIVATE Gpio.cpp)
target_link_libraries(Sts1CobcSw_Hal PUBLIC rodos::rodos type_safe)