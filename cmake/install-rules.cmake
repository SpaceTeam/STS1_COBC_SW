install(
    TARGETS CobcSw_HelloWorld
    RUNTIME COMPONENT CobcSw_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
