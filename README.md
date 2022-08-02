# STS1 COBC SW

This project contains the software of the communication and onboard computer (COBC) of Space Team
Satellite 1 (STS1).


## Building and hacking

Here is some wisdom to help you build this project as a developer. It uses CMake, assumes that you
are developing on Linux and that the required compilers and tools are installed. See [this wiki
page](https://wiki.tust.at/books/sts1/page/setup-compilers-and-tools#bkmrk-gnu-arm-embedded-too) for
more information on the latter.


### Dependencies

The following libraries are necessary to build the COBC SW.

- Rodos
- ETL (Embedded Template Library)
- type_safe

Quick installation instructions for those can be found
[here](https://wiki.tust.at/books/sts1/page/setup-libraries).


### Toolchain files

Generally, toolchain files are specific to the target platforms as well as your environment and
setup but not to your project. Therefore they are not supplied with this repo and should be kept at
some "global" directory (I, e.g., use `~/programming/cmake/`). A toolchain file for an STM32F411 can
look something like the following:

```cmake
# ##################################################################################################
# Cross compiler toolchain
# ##################################################################################################

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(TOOLCHAIN_PREFIX arm-none-eabi-)

# Find path to the cross compiler toolchain
execute_process(
    COMMAND which ${TOOLCHAIN_PREFIX}gcc
    OUTPUT_VARIABLE BINUTILS_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Use newlib nano because it is smaller
set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nano.specs")

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)

set(CMAKE_OBJCOPY
    ${TOOLCHAIN_PREFIX}objcopy
    CACHE INTERNAL "objcopy tool"
)
set(CMAKE_OBJDUMP
    ${TOOLCHAIN_PREFIX}objdump
    CACHE INTERNAL "objdump tool"
)
set(CMAKE_SIZE_UTIL
    ${TOOLCHAIN_PREFIX}size
    CACHE INTERNAL "size tool"
)

set(CMAKE_FIND_ROOT_PATH ${BINUTILS_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# ##################################################################################################
# Platform specific configuration
# ##################################################################################################

# Root folder containing platform specific stuff like libraries
set(platform_root "/usr/local/stm32f411")
list(APPEND CMAKE_FIND_ROOT_PATH "${platform_root}")
# For some reason the toolchain file always runs twice, so REMOVE_DUPLICATES is used to get rid of
# the 2. platform_root that gets appended
list(REMOVE_DUPLICATES CMAKE_FIND_ROOT_PATH)

set(linker_script "${platform_root}/src/rodos/src/bare-metal/stm32f4/scripts/stm32f411xe_flash.ld")
message("Linker script used: ${linker_script}")

# TODO: Find out why if(NOT DEFINED HSE_VALUE) does not work as expected and fails the second time
# the toolchain file is run
message("HSE value used: ${HSE_VALUE}")
add_compile_definitions(HSE_VALUE=${HSE_VALUE} HSE_STARTUP_TIMEOUT=10000000)
add_compile_definitions(USE_STDPERIPH_DRIVER STM32F411xE)

set(compile_and_link_options -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp)

add_compile_options(${compile_and_link_options})
add_compile_options(-gdwarf-2 -mthumb -g3)

add_link_options(${compile_and_link_options})
add_link_options(-Wl,-T${linker_script})
add_link_options(
    -nostartfiles -Xlinker --gc-sections -fno-unwind-tables -fno-asynchronous-unwind-tables
)
```

You must at least change the `platform_root` variable to point to the directory in which you install
all your cross-compiled libraries for the target platform (=STM32F411).


### Presets

This project makes use of [CMake
presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) to simplify the process of
configuring the project. As a developer, you should create a `CMakeUserPresets.json` file at the
root of the project that looks something like the following:

```json
{
  "version": 3,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 22,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "dev-common",
      "hidden": true,
      "inherits": [
        "dev-mode",
        "clang-tidy",
        "cppcheck",
        "ci-unix"
      ],
      "generator": "Ninja",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
        "BUILD_MCSS_DOCS": "ON"
      }
    },
    {
      "name": "dev-linux-x86",
      "binaryDir": "${sourceDir}/build/linux-x86",
      "inherits": "dev-common",
      "toolchainFile": "/usr/local/src/rodos/cmake/port/linux-x86.cmake"
    },
    {
      "name": "dev-cobc",
      "binaryDir": "${sourceDir}/build/cobc",
      "inherits": "dev-common",
      "toolchainFile": "~/programming/cmake/stm32f411.cmake",
      "cacheVariables": {
        "HSE_VALUE": "12000000"
      }
    },
    {
      "name": "dev-coverage",
      "binaryDir": "${sourceDir}/build/coverage",
      "inherits": [
        "dev-mode",
        "coverage-unix"
      ],
      "toolchainFile": "/usr/local/src/rodos/cmake/port/linux-x86.cmake"
    }
  ],
  "buildPresets": [
    {
      "name": "dev-linux-x86",
      "configurePreset": "dev-linux-x86",
      "configuration": "Debug",
      "jobs": 4
    },
    {
      "name": "dev-cobc",
      "configurePreset": "dev-cobc",
      "configuration": "Debug",
      "jobs": 4
    }
  ],
  "testPresets": [
    {
      "name": "dev-linux-x86",
      "configurePreset": "dev-linux-x86",
      "configuration": "Debug",
      "output": {
        "outputOnFailure": true
      },
      "execution": {
        "jobs": 4
      }
    }
  ]
}
```

The path to the toolchain files depend on your setup. The number of jobs given in the build and test
presets must be adapted by you as well and should ideally be set to the number of threads available
on your CPU.

In general, `CMakeUserPresets.json` is the perfect place in which you can put all sorts of things
that you would otherwise want to pass to the configure command in the terminal.


### Configure, build and test on Linux

If you followed the above instructions, then you can configure, build and test the project on Linux
respectively with the following commands from the project root:

```sh
cmake --preset=dev-linux-x86
cmake --build --preset=dev-linux-x86
ctest --preset=dev-linux-x86
```

To run the code execute

```sh
./build/linux-x86/CobcSw
```


### Configure, build for and flash onto the COBC

TBD


# Licensing

See the [LICENSE](LICENSE.md) document.
