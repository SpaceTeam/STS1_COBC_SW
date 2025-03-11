target_sources(Sts1CobcSw_Telemetry PRIVATE TelemetryRecord.cpp)
target_link_libraries(
    Sts1CobcSw_Telemetry PUBLIC Sts1CobcSw_FramSections Sts1CobcSw_Periphery Sts1CobcSw_ProgramId
                                Sts1CobcSw_Serial Sts1CobcSw_Utility
)
