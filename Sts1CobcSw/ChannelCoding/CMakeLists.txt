add_subdirectory(External)

target_sources(Sts1CobcSw_ChannelCoding PRIVATE ChannelCoding.cpp ReedSolomon.cpp Scrambler.cpp)
target_link_libraries(
    Sts1CobcSw_ChannelCoding PUBLIC External::ConvolutionalCoding Sts1CobcSw_Outcome
                                    Sts1CobcSw_Serial
)
target_link_libraries(Sts1CobcSw_ChannelCoding PRIVATE libfec::libfec)
if(DISABLE_CHANNEL_CODING)
    target_compile_definitions(Sts1CobcSw_ChannelCoding PUBLIC DISABLE_CHANNEL_CODING)
endif()
if(DISABLE_CONVOLUTIONAL_CODING)
    target_compile_definitions(External_ConvolutionalCoding PUBLIC DISABLE_CONVOLUTIONAL_CODING)
endif()
