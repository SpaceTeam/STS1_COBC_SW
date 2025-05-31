target_sources(
    Sts1CobcSw_Bootloader PRIVATE BusyWait.cpp Leds.cpp Main.cpp MemoryInitialization.cpp Startup.s
                                  UciUart.cpp
)
