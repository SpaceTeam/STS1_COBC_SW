target_sources(Sts1CobcSw_ChannelCoding PRIVATE ChannelCoding.cpp ReedSolomon.cpp Scrambler.cpp)
target_link_libraries(Sts1CobcSw_ChannelCoding PUBLIC Sts1CobcSw_Outcome Sts1CobcSw_Serial)
target_link_libraries(Sts1CobcSw_ChannelCoding PRIVATE libfec::libfec)
