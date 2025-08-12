target_sources(
    Sts1CobcSw_Bootloader
    PRIVATE BusyWait.cpp
            Leds.cpp
            Main.cpp
            MemoryInitialization.cpp
            Print.cpp
            RunFirmware.cpp
            Spi.cpp
            Startup.s
            UciUart.cpp
)
target_compile_definitions(
    Sts1CobcSw_Bootloader PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:ENABLE_DEBUG_PRINT>
)
target_link_libraries(Sts1CobcSw_Bootloader PRIVATE Sts1CobcSw_Serial)
