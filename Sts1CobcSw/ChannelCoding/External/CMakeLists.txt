add_library(External_ConvolutionalCoding STATIC)
add_library(External::ConvolutionalCoding ALIAS External_ConvolutionalCoding)
target_sources(External_ConvolutionalCoding PRIVATE ConvolutionalCoding.cpp)
target_link_libraries(External_ConvolutionalCoding PUBLIC etl::etl Sts1CobcSw_Serial)
if(DISABLE_CHANNEL_CODING)
    target_compile_definitions(External_ConvolutionalCoding PUBLIC DISABLE_CHANNEL_CODING)
endif()
