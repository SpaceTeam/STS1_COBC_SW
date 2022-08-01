install(
    TARGETS CobcSw_exe
    RUNTIME COMPONENT CobcSw_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
