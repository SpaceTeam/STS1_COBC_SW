add_library(Sts1CobcSwTests_Utility STATIC)
add_library(Sts1CobcSwTests::Utility ALIAS Sts1CobcSwTests_Utility)
target_sources(Sts1CobcSwTests_Utility PRIVATE Stringification.cpp)
target_link_libraries(
    Sts1CobcSwTests_Utility PUBLIC Sts1CobcSwTests::CatchRodos Sts1CobcSw_Outcome
                                   Sts1CobcSw_Vocabulary
)
target_include_directories(Sts1CobcSwTests_Utility PUBLIC "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>")
