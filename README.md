# STS1 COBC SW

This project contains the software of the communication and onboard computer (COBC) of
Space Team Satellite 1 (STS1).


## Building and hacking

### With Docker containers

The easiest way to build this project is with [Docker](https://www.docker.com/). The
[tuwienspaceteam/sts1-cobc](https://hub.docker.com/r/tuwienspaceteam/sts1-cobc) image is
specifically designed for that purpose. It comes with all required compilers, toolchains,
libraries and tools. If you don't want to cross-compile and are only interested in running
the software and tests on Linux than use the tags with a `-linux-x86` suffix. They are
quite a bit smaller because they do not have the GNU ARM Toolchain installed.

<!-- TODO: More details on how to use the Docker container. -->


### Without Docker containers

If you don't want to use Docker containers but install and build everything locally on
your machine here is some wisdom to help you with that. The project uses CMake, assumes
that you are developing on Linux and that the required compilers and tools are installed.
See [this wiki
page](https://wiki.tust.at/books/sts1/page/setup-compilers-and-tools#bkmrk-gnu-arm-embedded-too)
for more information on the latter.

Even when not using the Docker image, it is still very instructive to look at the Docker
files
([here](https://github.com/SpaceTeam/STS1_COBC_Docker/blob/master/linux-x86/Dockerfile)
and [here](https://github.com/SpaceTeam/STS1_COBC_Docker/blob/master/full/Dockerfile)) to
see how to properly install the compilers, tools and dependencies.


### Dependencies

The following libraries are necessary to build the COBC SW.

- Rodos
- ETL (Embedded Template Library)
- type_safe

Quick installation instructions for those can be found
[here](https://wiki.tust.at/books/sts1/page/setup-libraries).


### Toolchain files

Toolchain files are used when cross-compiling and are specific to your target platform,
environment and setup but not to your project. Therefore they are not supplied with this
repo and should be kept at some "global" directory (I, e.g., use `~/programming/cmake/`).
The repo of the sts1-cobc Docker image contains an appropriate [toolchain file targeting
an
STM32F411](https://github.com/SpaceTeam/STS1_COBC_Docker/blob/master/full/stm32f411.cmake).
You can use this as a template for your own toolchain files. Just change the
`platform_root` variable to point to the directory where you install all your
cross-compiled libraries for the target platform.


### Presets

This project makes use of [CMake
presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) to simplify the
process of configuring the project. As a developer, you should create a
`CMakeUserPresets.json` file at the root of the project that looks something like the
following:

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

The path to the toolchain files depend on your setup. The number of jobs given in the
build and test presets must be adapted by you as well and should ideally be set to the
number of threads available on your CPU.

In general, `CMakeUserPresets.json` is the perfect place in which you can put all sorts of
things that you would otherwise want to pass to the configure command in the terminal.


### Configure, build and test locally on Linux

If you followed the above instructions, then you can configure, build and test the project
on your local Linux machine respectively with the following commands from the project
root:

```sh
cmake --preset=dev-linux-x86
cmake --build --preset=dev-linux-x86
cmake --build build/linux-x86 -t Tests
ctest --preset=dev-linux-x86
```

To run the code execute

```sh
./build/linux-x86/CobcSw
```


### Configure, build for and flash onto the COBC

TBD

```sh
cmake --preset=dev-cobc
cmake --build --preset=dev-cobc
```


# Licensing

See the [LICENSE](LICENSE) document.
