target_sources(
    Sts1CobcSw_Bootloader
    PRIVATE BusyWait.cpp
            Fram.cpp
            Leds.cpp
            Main.cpp
            MemoryInitialization.cpp
            RunFirmware.cpp
            Spi.cpp
            Startup.s
            UciUart.cpp
)
target_compile_definitions(
    Sts1CobcSw_Bootloader PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:ENABLE_DEBUG_PRINT>
)
target_link_libraries(
    Sts1CobcSw_Bootloader PRIVATE Sts1CobcSw_ErrorDetectionAndCorrection Sts1CobcSw_Serial
                                  Sts1CobcSw_Utility Sts1CobcSw_Vocabulary
)
