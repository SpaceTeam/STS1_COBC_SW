target_sources(Sts1CobcSw_Telemetry PRIVATE TelemetryRecord.cpp)
target_link_libraries(
    Sts1CobcSw_Telemetry PUBLIC Sts1CobcSw_FirmwareManagement Sts1CobcSw_FramSections
                                Sts1CobcSw_Sensors Sts1CobcSw_Serial Sts1CobcSw_Vocabulary
)
