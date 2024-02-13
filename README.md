# STS1 COBC SW

This project contains the software for the communication and onboard computer (COBC) of
Space Team Satellite 1 (STS1).

## Contents

- [Building and developing](#building-and-developing)
- [Project layout](#project-layout)
- [Contributing](#contributing)
- [Licensing](#licensing)


## Building and developing

The recommended way to develop and build this project is to use the Docker image
[tuwienspaceteam/sts1-cobc](https://hub.docker.com/r/tuwienspaceteam/sts1-cobc) as a dev
container. It is specifically built for that purpose, i.e., it comes with all required
compilers, libraries and tools. This makes it easier to get started but also ensures
reliable and consistent builds. See [this
page](https://wiki.tust.at/books/spaceteamsat1-sts1/page/setup-compilers-and-tools) on our
internal wiki for more information.

If you don't want to use Docker, it is still best to check out the Dockerfiles
([here](https://github.com/SpaceTeam/STS1_COBC_Docker/blob/master/linux-x86/Dockerfile)
and [here](https://github.com/SpaceTeam/STS1_COBC_Docker/blob/master/full/Dockerfile)) to
see how to properly install everything.


### Presets

This project makes use of [CMake
presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) to simplify the
process of configuring the project. As a developer, you should create a
`CMakeUserPresets.json` file at the top-level directory of the repository. If you use the
Docker image, the file should look something like the following:

<details>
  <summary>CMakeUserPresets.json</summary>

  ~~~json
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
          "ci-unix",
          "include-what-you-use"
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
        "toolchainFile": "/linux-x86.cmake"
      },
      {
        "name": "dev-cobc",
        "binaryDir": "${sourceDir}/build/cobc",
        "inherits": "dev-common",
        "toolchainFile": "/stm32f411.cmake",
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
        "toolchainFile": "/linux-x86.cmake"
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

  ~~~

</details>

The paths to the toolchain files depend on your setup. If you don't use Docker you mostly
likely have to change them. The number of jobs given in the build and test presets must be
adapted by you as well and should ideally be set to the number of threads available on
your CPU. In general, `CMakeUserPresets.json` is the perfect place in which you can put
all sorts of things that depend on your personal setup or preference, and that you would
otherwise want to pass to the CMake command in the terminal.


### Configure, build and test

The following instructions assume that you added the above `CMakeUserPresets.json` and
that the commands are executed from within the Docker container. This is easy with VS Code
since it allows directly [developing inside a
container](https://code.visualstudio.com/docs/devcontainers/containers). If you don't use
VS Code you must execute all commands via `docker run`. In this case it is convenient to use an alias like the following:

~~~shell
# Version 1: always mounts the STS1_COBC_SW folder
alias dr-sts1="docker run -it -v /path/to/STS1_COBC_SW:/project -w='/project' tuwienspaceteam/sts1-cobc:0.7.0"

# Version 2: mounts the current working directory. This means that you must be in the
# top-level directory of the COBC SW repo when executing the build commands.
alias dr-sts1="docker run -it -v $(pwd):/project -w='/project' tuwienspaceteam/sts1-cobc:0.7.0"
~~~

You can configure, build and test the `linux-x86` parts of the project with the following
commands:

~~~shell
cmake --preset=dev-linux-x86
cmake --build --preset=dev-linux-x86
cmake --build build/linux-x86 -t Tests
ctest --preset=dev-linux-x86
~~~

To run a dummy program on Linux execute

~~~shell
./build/linux-x86/HelloDummy
~~~

To cross-compile for the COBC run

~~~shell
cmake --preset=dev-cobc
cmake --build --preset=dev-cobc
~~~

If you want to build a specific target, e.g., the hardware test for the FRAM you must
execute

~~~shell
cmake --prest=dev-cobc
cmake --build --preset=dev-cobc --target Sts1CobcSwTests_Fram
~~~


## Project layout

The following ideas are mainly stolen from [P1204R0 – Canonical Project
Structure](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1204r0.html).

- "All" file and folder names uses UpperCamelCase (unfortunately cmake-init uses different
  cases, so this is a lot to change and `docs/` or `cmake/` might stay in the wrong case
  for a while).
- Top level directory is project name in UpperCamelCase or GitHub repo name (unfortunately
  we use a different naming convention with SHOUTING_CASE on GitHub).
- Source code is in subdirectory named after the project (no GitHub name this time and no
  `Source` or `Include` folders).
- The project namespace is called `sts1cobcsw`.
- Only the top level directory is added to the include path so that all includes must
  spell out the directory structure, e.g., `#include <Sts1CobcSw/Hal/IoNames.hpp>`.
- Also, all includes, even the "project local" ones use `<>` instead of `""`.
- Subfolders for the source code should comprise somewhat standalone "components".
- There should be a STATIC or INTERFACE library target for each component (this should
  make linking source code dependencies for tests easier).
- Each subfolder should have its own `CMakeLists.txt`
- Rodos already provides some kind of hardware abstraction. `Hal/` should therefore
  provide type-safe and more specifically named C++ wrappers for this low-level Rodos
  stuff.
- `Periphery/` should contain abstractions for the external periphery the COBC
  communicates with, like sensors or memory chips. The EDU also kinda fits here.
- All CMake targets are prefixed with the project name: `Sts1CobcSw_CobcSw`,
  `Sts1CobcSw_Hal`, etc.
- Everything test related is in `Tests/` and its subdirectories.
- Tests have the `.test.cpp` extension (no `_Test` suffix or `Test_` prefix, etc.) and are
  named after the class, file, functionality, interface or whatever else they test.
- Hardware tests should be similar to unit tests and check simple functionalities of
  low-level code. They should also be somewhat automatable with the help of the whole
  FlatSat setup (UCI UART, LabJack, etc.)
- Golden Tests are used for high level integration/system tests. We should be able to run
  them on the FlatSat too, because `PRINTF()` prints to the UCI UART which is connected to
  the PC.

The following shows what the directory structure could actually look like.

<details>
  <summary>Directory structure</summary>

  ~~~
  Sts1CobcSw/
  ├── .github/
  ├── CMake/
  ├── Docs/
  ├── Sts1CobcSw/
  │   ├── Hal/
  │   │   ├── PinNames.hpp
  │   │   ├── IoNames.hpp
  │   │   ├── Usart.cpp
  │   │   ├── Usart.hpp
  │   │   ├── Spi.cpp
  │   │   ├── Spi.hpp
  │   │   ├── Communication.hpp   (maybe just this instead?)
  │   │   └── ...
  │   │
  │   ├── Periphery/
  │   │   ├── Edu.cpp
  │   │   ├── Edu.hpp
  │   │   ├── W25q01jvzeiq.cpp  (Name of the flash chip)
  │   │   ├── W25q01jvzeiq.hpp
  │   │   ├── AnotherChipName.cpp
  │   │   ├── AnotherChipName.hpp
  │   │   └── ...
  │   │
  │   ├── ShouldThisEvenBeInASubfolder/
  │   │   ├── TelemetryMemory.cpp
  │   │   ├── TelemetryMemory.hpp
  │   │   ├── CobcFileSystem.cpp
  │   │   ├── CobcFileSystem.hpp
  │   │   ├── PersistantState.cpp
  │   │   ├── PersistantState.hpp
  │   │   └── ...
  │   │
  │   ├── AntennaDeploymentThread.cpp
  │   ├── AntennaDeploymentThread.hpp
  │   ├── SensorThread.cpp
  │   ├── SensorThread.hpp
  │   ├── CommandParser.cpp
  │   ├── CommandParser.hpp
  |   └── ...
  │
  ├── Tests/
  |   ├── GoldenTests/
  │   │   ├── ExpectedOutputs/
  │   │   ├── Scripts/
  │   │   ├── ICantThingOfAGoodName.test.cpp
  │   │   └── ...
  │   ├── HardwareTests/
  │   │   ├── GpioPins.test.cpp
  │   │   ├── Usart.test.cpp
  │   │   ├── W25q01jvzeiq.test.cpp
  │   │   └── ...
  │   ├── UnitTests/
  │   │   ├── CommandParser.test.cpp
  │   │   └── ...
  │   └── ...
  │
  ├── .clang-format
  ├── .gitignore
  ├── CMakeLists.txt
  ├── CMakePresets.json
  ├── LICENSE
  ├── README.md
  └── ...
  ~~~

</details>


## Contributing

Only contributions from members of the TU Wien Space Team are accepted.


## Licensing

See the [LICENSE](LICENSE) document.
