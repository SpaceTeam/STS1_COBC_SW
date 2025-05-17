target_sources(
    Sts1CobcSw_RfProtocols PRIVATE MessageTypeIdFields.cpp Reports.cpp SpacePacket.cpp
                                   TcTransferFrame.cpp TmTransferFrame.cpp
)
target_link_libraries(
    Sts1CobcSw_RfProtocols PUBLIC etl::etl rodos::without_main_on_linux Sts1CobcSw_FileSystem
                                  Sts1CobcSw_Outcome Sts1CobcSw_Serial Sts1CobcSw_Telemetry
)
target_link_libraries(Sts1CobcSw_RfProtocols PRIVATE Sts1CobcSw_RealTime)
