# -

## Which file is compiled?

Open the repository root as the CMake project. The firmware entry point is
`CBoard_Basics_Complete/Src/main.c`, and its build configuration is
`CBoard_Basics_Complete/CMakeLists.txt`. The root `.vscode/settings.json` points
VS Code CMake Tools at the root wrapper, which adds the firmware project.

`代码/main.c` is a reference snippet for copying application calls into a
CubeMX project. It is not part of the CMake target and cannot compile by itself:
it depends on the generated HAL headers, peripheral handles, and initialization
functions in `CBoard_Basics_Complete`.

To build, install CMake 3.22+, Ninja, and GNU Arm Embedded Toolchain, then open
the repository root and use the `STM32F407 Debug` configure and `Debug` build
presets in VS Code CMake Tools. `Ctrl+Shift+B` runs the default task, which
configures and builds that preset. The generated firmware is
`CBoard_Basics_Complete/build/Debug/CBoard_Basics_Complete.elf`.
