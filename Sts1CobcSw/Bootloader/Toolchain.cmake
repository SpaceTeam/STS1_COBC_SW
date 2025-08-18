# ##################################################################################################
# Cross compiler toolchain
# ##################################################################################################

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(TOOLCHAIN_PREFIX arm-none-eabi-)

# Find path to the cross compiler toolchain
execute_process(
    COMMAND which ${TOOLCHAIN_PREFIX}gcc OUTPUT_VARIABLE BINUTILS_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)

set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy CACHE INTERNAL "objcopy tool")
set(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}objdump CACHE INTERNAL "objdump tool")
set(CMAKE_SIZE_UTIL ${TOOLCHAIN_PREFIX}size CACHE INTERNAL "size tool")

set(CMAKE_FIND_ROOT_PATH ${BINUTILS_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# ##################################################################################################
# Platform specific configuration
# ##################################################################################################

# The Ninja Multi-Config generator does not support the MinSizeRel configuration by default so we
# add it here.
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel")
# I thought setting the default flags for the configurations is done with the *_INIT variables but
# that prepends to the existing default flags instead of replacing them.
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -gdwarf-2")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g3 -gdwarf-2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -flto -DNDEBUG")

set(compile_and_link_options -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp
                             --specs=nano.specs
)

add_compile_options(
    ${compile_and_link_options}
    -mthumb
    -ffunction-sections
    -fdata-sections
    -fno-exceptions
    -fno-non-call-exceptions
    -fno-rtti
)

add_link_options(${compile_and_link_options})
add_link_options(
    -nostartfiles --specs=nosys.specs -Xlinker --gc-sections -fno-unwind-tables
    -fno-asynchronous-unwind-tables
)
