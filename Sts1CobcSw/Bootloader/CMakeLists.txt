target_sources(
    Sts1CobcSw_Bootloader
    PRIVATE BusyWait.cpp
            Leds.cpp
            Main.cpp
            MemoryInitialization.cpp
            RunFirmware.cpp
            Startup.s
            UciUart.cpp
)
