# STS1 COBC SW

This project contains the software for the communication and onboard computer (COBC) of
SpaceTeamSat1 (STS1).

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
process of configuring the project.

As a developer, you should create a `CMakeUserPresets.json` file at the root of the
project. If you use the Docker image,
[`CMakeDeveloperPresets.json`](CMakeDeveloperPresets.json) already contains a convenient
set of configure and build presets for you. Just include it in your user preset file as
shown in the following example.

<details>
  <summary>CMakeUserPresets.json</summary>

  ~~~json
  {
    "version": 4,
    "cmakeMinimumRequired": {
      "major": 3,
      "minor": 23,
      "patch": 0
    },
    "include": [
      "CMakeDeveloperPresets.json"
    ]
  }
  ~~~

</details>

The paths to the toolchain files depend on your setup. If you don't use Docker you mostly
likely have to change them. In general, `CMakeUserPresets.json` is the perfect place in
which you can put all sorts of things that depend on your personal setup or preference,
and that you would otherwise want to pass to the CMake command in the terminal.


### Include What You Use

To ensure that source files include only the header files they need, we use [Include What
You Use (IWYU)](https://github.com/include-what-you-use/include-what-you-use), a tool
built on top of Clang. IWYU supports a [mapping
file](https://github.com/include-what-you-use/include-what-you-use/blob/master/docs/IWYUMappings.md)
for more precise configuration, allowing us to make sure it works the way we want it to.
In particular, this means that we have to do the following additional steps when adding a
header (`.hpp`) or inline implementation (`.ipp`) file:

- Add a line to the mappings file to ensure that the header file gets included with angle
  brackets instead of quotes. Unfortunately, this still doesn't stop IWYU from sometimes
  suggesting our includes with quotes. I don't know why that is.

  ~~~
  { include: ["\"Sts1CobcSw/Hal/Spi.hpp\"", "public", "<Sts1CobcSw/Hal/Spi.hpp>", "public"] },
  ~~~

- Add a line to the mappings file to ensure that the `.ipp` file is mapped to the
  corresponding `.hpp` file. This ensures that only the `.hpp` file gets included.

  ~~~
  { include: ["\"Sts1CobcSw/Hal/Spi.ipp\"", "private", "<Sts1CobcSw/Hal/Spi.hpp>", "public"] },
  ~~~

- Add a pragma when including the corresponding `.ipp` file in the `.hpp` file:

  ~~~
  #include <Sts1CobcSw/Hal/Spi.ipp>  // IWYU pragma: keep
  ~~~

The complete mapping file is called `iwyu.imp` and is located in the top-level directory.


### Configure, build and test

The following instructions assume that you added the above `CMakeUserPresets.json` and
that the commands are executed from within the Docker container. This is easy with VS Code
since it allows directly [developing inside a
container](https://code.visualstudio.com/docs/devcontainers/containers). If you don't use
VS Code you must execute all commands via `docker run`. In this case it is convenient to
use an alias like the following:

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
cmake --build --preset=dev-linux-x86-debug
cmake --build --preset=dev-linux-x86-debug -t AllTests
ctest --test-dir build/dev-linux-x86/Tests -C Debug
~~~

To cross-compile for the COBC run

~~~shell
cmake --preset=dev-cobc
cmake --build --preset=dev-cobc-debug
~~~

If you want to build a specific target, e.g., the hardware test for the FRAM you must
execute

~~~shell
cmake --prest=dev-cobc
cmake --build --preset=dev-cobc-debug -t Sts1CobcSwTests_Fram
~~~

There are also build presets for non-debug builds. On Linux there is
`dev-linux-x86-release`, and for the COBC we have `dev-cobc-min-size-rel`.


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
  communicates with, like sensors or memory chips. The EDU also kind of fits here.
- All CMake targets are prefixed with the project name: `Sts1CobcSw_Firmware`,
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


## Licensing and attribution

This project is licensed under the MIT license. See the [LICENSE](LICENSE) document for
the full text.

We do use some code from other projects, though, so here is a list with licensing and
attribution information for those parts.

- The files in `Sts1CobcSw/Blake2s/External/` were copied from
  [arduinolibs](https://github.com/rweather/arduinolibs/) which is also MIT licensed
- The lookup tables for our bit-scrambling code in
  [`Sts1CobcSw/ChannelCoding/Scrambler.cpp`](./Sts1CobcSw/ChannelCoding/Scrambler.cpp)
  were taken from [bitsnarl](https://github.com/Tagussan/bitsnarl)
