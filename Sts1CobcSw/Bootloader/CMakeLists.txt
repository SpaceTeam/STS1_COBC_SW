target_sources(
    Sts1CobcSw_Bootloader
    PRIVATE BusyWait.cpp
            Leds.cpp
            Main.cpp
            MemoryInitialization.cpp
            Print.cpp
            RunFirmware.cpp
            Startup.s
            UciUart.cpp
)
target_compile_definitions(
    Sts1CobcSw_Bootloader PUBLIC $<$<CONFIG:Debug,RelWithDebInfo>:ENABLE_DEBUG_PRINT>
)
