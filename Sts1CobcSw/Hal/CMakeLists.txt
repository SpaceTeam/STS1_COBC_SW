target_sources(Sts1CobcSw_Hal PRIVATE Gpio.cpp IoNames.hpp PinNames.hpp Communication.hpp)
target_link_libraries(Sts1CobcSw_Hal PRIVATE rodos::rodos etl::etl)